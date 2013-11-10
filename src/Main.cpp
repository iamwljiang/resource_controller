#include <stdio.h>
#include "lock.h"
#include "proc.h"
#include "os.h"
#include "strings.h"
#include "path.h"
#include "cgroups.h"

#include <iostream>

#include <set>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/wait.h>

static	std::string sub = "memory";
static	std::string mount_path = "/cgroup/memory";	
static  std::string cgroup = "submem";

int sub_proc()
{	
	std::vector<char*> mem_pool;
	int mem_unit = 1024*1024;
	int nmem = 0;
	sleep(1);
	while( nmem++ < 1000 ){
		char* buf = (char*)malloc(mem_unit);
		if( NULL == buf ){
			std::cout << "alloc buffer failed\n";
			break;
		}

		for(int i=0; i < mem_unit/4; ++i){
			((int*)buf)[i] = i;
		}

		mem_pool.push_back(buf);
		assert( 0 == usleep(10000) ) ;
		std::cout << "alloc " << nmem << "M\n";
	}

	std::cout << "total alloc memory " << mem_pool.size() << "M\n";

	size_t len = mem_pool.size();
	for( size_t i = 0; i < len; ++i ){
		free(mem_pool.at(i));
	}

	return 0;
}

bool check_enabled(const std::string& sub)
{
	if(!cgroups::enabled()){
		printf("cgroups not enabled\n");
		return false;
	}

	std::set<std::string> subsystems = cgroups::subsystems();
	if(subsystems.count(sub) < 0){
		printf("cgroups support subsystems not find subsystem:%s\n",sub.c_str());
		return false;
	}

	return true;
}

bool sub_process_with_fork()
{
	int fd[2];
	if(socketpair(AF_LOCAL,SOCK_STREAM,0,fd) < 0)
		return false;

	pid_t pid = fork();
	if(pid == 0){
		int go = 0;
		read(fd[1],(char*)&go,sizeof(int));
		close(fd[0]);
		close(fd[1]);

		//NOTE:at here assgin pid to task(writed) ,it be effective
		//int ret = cgroups::assign(mount_path,cgroup,getpid());

		if(0 == go)
			sub_proc();
		exit(0);
	}else if(pid > 0){
		std::cout << "create sub process,pid:" << pid << std::endl;
		int ret = 0 ;
		
		//NOTE:at here we assgin sub process pid to task(writed),it ok,but assgin parent pid it will not effective.
		ret = cgroups::assign(mount_path,cgroup,pid);
		//ret = cgroups::assign(mount_path,cgroup,getpid());

		if(0 == ret){
			std::string control = "memory.limit_in_bytes";
			std::string value = "100M";
			ret = cgroups::write(mount_path,cgroup,control,value);
			if(0 != ret){
				std::string rvalue = cgroups::read(mount_path,cgroup,control);
				std::cerr << "control:" << control << " write value:" << value << " read value:" << rvalue << std::endl;
			}

			control = "memory.memsw.limit_in_bytes";
			ret = ret != 0 ? -1 : cgroups::write(mount_path,cgroup,control,value);
			if(0 != ret){
				std::string rvalue = cgroups::read(mount_path,cgroup,control);
				std::cerr << "control:" << control << " write value:" << value << " read value:" << rvalue << std::endl;
			}
		}

		//send command to subprocess
		write(fd[0],(char*)&ret,sizeof(int));

		close(fd[0]);
		close(fd[1]);
		waitpid(pid,NULL,0);
		printf("waitpid end\n");
	}else{
		return false;
	}
}

bool sub_process_with_exec(int argc,char**argv)
{
	int ret = 0 ;
	ret = cgroups::assign(mount_path,cgroup,getpid());
	if( ret < 0) return false;

	std::string control = "memory.limit_in_bytes";
	std::string value = "100M";
	ret = cgroups::write(mount_path,cgroup,control,value);
	if( ret < 0) return false;
	control = "memory.memsw.limit_in_bytes";
	ret = cgroups::write(mount_path,cgroup,control,value);
	if( ret < 0) return false;

	execvp("./TestMemory",argv);

	return true;
}

int main(int argc,char** argv)
{
	
	if(!check_enabled(sub))
		return -1;

	//if hierarchy mounted,umount it
	if(cgroups::mounted(mount_path,sub)){
		printf("already mounted in %s,try unmount it\n",mount_path.c_str());
		std::vector<std::string> groups = cgroups::get(mount_path);
		if(!groups.empty()){
			std::vector<std::string>::iterator g_iter,g_end;
			g_iter = groups.begin();
			g_end = groups.end();
			for(; g_iter != g_end; ++g_iter){
				if(cgroups::remove(mount_path,*g_iter) != 0){
					printf("remove exist cgroup failed:%s",(*g_iter).c_str());
				}
			}
		}

		if(cgroups::unmount(mount_path) != 0){
			printf("unmount hierarchy:%s failed\n",mount_path.c_str());
			return -1;
		}
	}
	
	if(cgroups::mount(mount_path,sub,3) != 0){
		printf("mount hierarchy :%s of subsystem:%s failed\n",mount_path.c_str(),sub.c_str());
		return -1;
	}else{
		printf("mount hierarchy :%s of subsystem:%s success\n",mount_path.c_str(),sub.c_str());
	}
	
	//check memory subsystem already in hierachy
	std::set<std::string> subsystems = cgroups::subsystems(mount_path);
	if(subsystems.count("memory") == 0){
		printf("mount hierachy not exist 'memory' subsystem\n");
		return -1;
	}

	//create cgroup
	if(cgroups::create(mount_path,cgroup) < 0){
		printf("create cgroup failed\n");
		return -1;
	}

	sub_process_with_fork();
	//sub_process_with_exec(argc,argv);


	printf("successful\n");
    return 0;
}
