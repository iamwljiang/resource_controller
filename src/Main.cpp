#include <stdio.h>
#include "lock.h"
#include "proc.h"
#include "os.h"
#include "strings.h"
#include "path.h"
#include "cgroups.h"

#include <iostream>



#include <iostream>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

int sub_proc()
{	
	std::vector<char*> mem_pool;
	int mem_unit = 1024*1024;
	int nmem = 0;
	sleep(20);
	while( nmem++ < 1024 ){
		char* buf = new char[mem_unit];
		if( NULL == buf ){
			std::cout << "alloc buffer failed\n";
			continue;
		}

		mem_pool.push_back(buf);
		time_t b,e;
		b = time(NULL);
		assert( 0 == usleep(100000) ) ;
		e = time(NULL);
		std::cout << "cost time:" << e - b << "sec\n";
	}

	std::cout << "alloc memeory " << mem_pool.size() << "M\n";

	size_t len = mem_pool.size();
	for( int i = 0; i < len; ++i ){
		delete [] mem_pool.at(i);
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

	if(cgroups::busy(sub)){
		printf("subsystem:%s is busy\n",sub.c_str());
		return false;
	}

	return true;
}


int main(int argc,char** argv)
{
	std::string sub = "memory";
	std::string mount_path = "/cgroup/memory";	
	
	//if(!check_enabled(sub))
	//	return -1;

	if(cgroups::mounted(mount_path,sub)){
		printf("already mounted in %s,try unmount it\n",mount_path.c_str());
		if(!cgroups::unmount(mount_path)){
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


	printf("successful\n");
    return 0;
}
