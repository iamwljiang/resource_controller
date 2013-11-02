#ifndef __MEMORY_CONTROLLER_CGROUP_H__
#define __MEMORY_CONTROLLER_CGROUP_H__

#include <stdint.h>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <sys/types.h>

namespace cgroups{

//系统是否支持cgroup
bool enabled();
//获取当前活跃的层级
int hierarchies(std::set<std::string>& result);
//获取子系统附在哪个已经mount的层级
std::string hierarchy(const std::string& subsystems);
//查看当前系统是否支持这个子系统
bool enabled(const std::string& subsystems);
//查看子系统是否附加到了层级上
bool busy(const std::string& subsystems);
//返回/proc/cgroups中存在的子系统，即系统支持的子系统
std::set<std::string>  subsystems();
//返回所有附加在某个指定层级上的子系统
std::set<std::string>  subsystems(const std::string& hierarchy);
//挂载控制族群到某个层级,并附加子系统
int mount(const std::string& hierarchy, const std::string& subsystems, int retry);
//卸载层级并移除相关的目录
int unmount(const std::string& hierarchy);
//判断给定的根层级是被挂载到虚拟文件系统中,并且指定的子系统有附加在上面
bool mounted(const std::string& hierarchy, const std::string& subsystems="");
//在某个层级下面创建控制控制族群
int create(const std::string& hierarchy, const std::string& cgroup);
//移除某个层级下的控制族群
int remove(const std::string& hierarchy, const std::string& cgroup);
//返回在给定的层级的控制族群下的所有控制族群，默认返回层级下的所有控制族群
std::vector<std::string> get(const std::string& hierarchy, const std::string& cgroup = "/");
//判断在指定的层级下是否有指定的控制族群
bool exists(const std::string& hierarchy, const std::string& cgroup);
//发送指定的信号给在指定控制族群中的所有进程
//int kill(const std::string& hierarchy,const std::string& cgroup,int signal);
//读取控制文件
std::string read(const std::string& hierarchy,const std::string& cgroup,const std::string& control);
//写控制文件
int write(const std::string& hierarchy,const std::string& cgroup,const std::string& control,const std::string& value);
//判断在指定层级下的控制族群中的控制文件是否有效
bool exists(const std::string& hierarchy,const std::string& cgroup,const std::string& control);
//获取在指定层级下的属于指定控制族群的所有进程号列表
std::set<pid_t> tasks(const std::string& hierarchy, const std::string& cgroup);
//将指定的进程的pid赋给指定的控制族群
int assign(const std::string& hierarchy, const std::string& cgroup, pid_t pid);
//返回给定文件的所有信息
std::map<std::string, uint64_t> stat(const std::string& hierarchy,const std::string& cgroup,const std::string& file);
//清除层级,先销毁所有相关的控制族群,卸载层级,删除挂载点
bool cleanup(const std::string& hierarchy);

} //namespace cgroups

#endif //__MEMORY_CONTROLLER_CGROUP_H__
