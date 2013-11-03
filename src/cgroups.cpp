#include "cgroups.h"
#include "os.h"
#include "path.h"
#include "strings.h"
#include "proc.h"
#include <fstream>
#include <assert.h>

namespace cgroups{
namespace internel{

std::map<std::string,SubsystemInfo> subsystems()
{
    std::map<std::string,SubsystemInfo> infos;

    //找出文件中的记录
    std::ifstream in("/proc/cgroups");
    if(!in.is_open()){
        return infos;
    }

    while(!in.eof()){
        std::string line;
        std::getline(in,line);
        
        //流出现错误且不是位于文件尾则关闭文件返回
        if(in.fail()){
            if(!in.eof()){
                in.close();
                return infos;
            }
        }else{
            if(line.empty()){
                continue;
            }else if(line.find_first_of('#') == 0){
                continue;
            }else{
                std::string name;
                int hierarchy;
                int cgroups;
                bool enabled;
                std::istringstream ss(line);
                ss >> std::dec >> name >> hierarchy >> cgroups >> enabled;

                //如果解析出错,则返回
                if(ss.fail() && !ss.eof()){
                    in.close();
                    return infos;
                }

                infos[name] = SubsystemInfo(name,hierarchy,cgroups,enabled);
            }
        }
    }

    return infos;
}

//挂载需要层级不存在,子系统可用,且不没有附加到某个层级中
int mount(const std::string& hierarchy, const std::string& subsystems)
{
    if(os::exists(hierarchy)){
        return -1;
    }

    std::vector<std::string> subsystems_items = strings::tokenize(subsystems, ",");
    size_t i = 0;
    size_t n = subsystems_items.size();
    for(; i < n; ++i){
        std::string& subsystem = subsystems_items.at(i);
        if(!enabled(subsystem)){
            return -1;
        }

        if(busy(subsystem)){
            return -1;
        }
    }

    if(!os::mkdir(hierarchy)){
        return -1;
    }

    int ret = ::mount(subsystems.c_str(),hierarchy.c_str(),"cgroup",0,subsystems.c_str());
    return ret;
}

int unmount(const std::string& target,int flag = 0)
{
    return ::umount2(target.c_str(),flag);
}

static int remove(const std::string& hierarchy, const std::string& cgroup)
{
	std::string path = path::join(hierarchy, cgroup);

  // Do NOT recursively remove cgroups.
  bool rmdir = os::rmdir(path, false);

  if (!rmdir) {
    return -1;
  }

  return 0;
}

static std::string read(
    const std::string& hierarchy,
    const std::string& cgroup,
    const std::string& control)
{
  std::string path = path::join(hierarchy, cgroup, control);

  // TODO(benh): Use os::read. Note that we do not use os::read
  // currently because it cannot correctly read /proc or cgroups
  // control files since lseek (in os::read) will return error.
  std::ifstream file(path.c_str());

  if (!file.is_open()) {
    return "";
  }

  std::ostringstream ss;
  ss << file.rdbuf();

  if (file.fail()) {
    // TODO(jieyu): Does std::ifstream actually set errno?
    file.close();
    return "";
  }

  file.close();
  return ss.str();
}

static int write(
    const std::string& hierarchy,
    const std::string& cgroup,
    const std::string& control,
    const std::string& value)
{
  std::string path = path::join(hierarchy, cgroup, control);
  std::ofstream file(path.c_str());

  if (!file.is_open()) {
    return -1;
  }

  file << value << std::endl;

  if (file.fail()) {
    // TODO(jieyu): Does std::ifstream actually set errno?
    file.close();
    return -1;
  }

  file.close();
  return 0;
}


} //namespace internel




//验证某个层级是否挂载,cgroup是否存在,控制文件是否存在
static bool verify(const std::string& hierarchy,const std::string& cgroup = "",const std::string& control = "")
{
    bool mounted = cgroups::mounted(hierarchy);
    if(!mounted){
        return false;
    }

    std::string cgroup_path;
    if(cgroup != ""){
        cgroup_path = path::join(hierarchy,cgroup);

        if(!os::exists(cgroup_path)){
            return false;
        }
    }

    if(control != ""){
        assert(cgroup != "");
        cgroup_path = path::join(cgroup_path,control);

        if(!os::exists(cgroup_path)){
            return false;
        }
    }

    return true;
}

bool enabled()
{
    return os::exists("/proc/cgroups");
}

int hierarchies(std::set<std::string>& results)
{   
    MountTable* table = MountTable::read("/proc/mounts");
    if(!table){
        return -1;
    }

    size_t n = table->entries.size();
    size_t i = 0;
    for(; i < n; ++i){
        struct MountTable::Entry& entry = table->entries.at(i);
        if("cgroup" == entry.type){
            std::string realpath = os::realpath(entry.dir);
            if(realpath.empty()){
                delete table,table = NULL;
                return -1;
            }
        
            results.insert(realpath);
		}
    }

    delete table,table = NULL;
    return 0;
}

std::string hierarchy(const std::string& subsystems)
{
    std::string hierarchy = "";
    std::set<std::string> result;
    if(hierarchies(result) < 0){
        return hierarchy;
    }

	std::set<std::string>::iterator iter,end;
	end = result.end();
    for(iter = result.begin(); iter != end; ++iter){
        std::string candidate = *iter;
        //如果没有子系统要查,则返回第一个层级
        if(subsystems.empty()){
            hierarchy = candidate;
        }

        bool mounted = cgroups::mounted(candidate,subsystems);
        if(mounted){
            hierarchy = candidate;
            break;
        }
    }

    return hierarchy;
}

bool mounted(const std::string& hierarchy, const std::string& subsystems)
{
    if(!os::exists(hierarchy))
        return false;

    std::string realpath = os::realpath(hierarchy);
    if(realpath.empty())
        return false;
    
    std::set<std::string> hierarchies;
    if(cgroups::hierarchies(hierarchies) < 0)
        return false;

    //判断绝对路径是否在层级列表中
    if(hierarchies.count(realpath) == 0)
        return false;

    std::set<std::string> attached = cgroups::subsystems(hierarchy);
    if(attached.empty())
        return false;

    std::vector<std::string> subsystems_items = strings::tokenize(subsystems,",");
    size_t n = subsystems_items.size();
    size_t i = 0;
    for(; i < n; ++i){
        std::string& subsystem = subsystems_items.at(i);
        if(attached.count(subsystem) == 0)
            return false;
    }
    
    return true;
}

std::set<std::string>  subsystems()
{
    std::set<std::string> results;
    std::map<std::string,SubsystemInfo> infos = internel::subsystems();

    //提取出enable的记录
    std::map<std::string,SubsystemInfo>::iterator iter,end;
    iter = infos.begin();
    end = infos.end();
    for(;iter != end; ++iter){
        if(iter->second.enabled){
            results.insert(iter->second.name);
        }
    }

    return results;
}

std::set<std::string>  subsystems(const std::string& hierarchy)
{
    std::set<std::string> results;
    std::string hierarchyAbsPath = os::realpath(hierarchy);
    if(hierarchyAbsPath.empty()){
        return results;
    }

    MountTable* table = MountTable::read("/proc/mounts");
    if(NULL == table){
        return results;
    }

    MountTable::Entry hierarchyEntry;
    bool found = false;
    size_t n = table->entries.size();
    size_t i = 0;
    for(; i < n; ++i){
        MountTable::Entry& entry = table->entries.at(i);
        if("cgroup" == entry.type){
            std::string dirAbsPath = os::realpath(entry.dir);
            if(dirAbsPath.empty()){
                delete table,table = NULL;
                return results;
            }

            if(dirAbsPath == hierarchyAbsPath){
                hierarchyEntry = entry;
                found = true;
            }
        }
    }

    //如果指定的层级没有在挂载列表中出现则返回
    if(!found){
        delete table,table = NULL;
        return results;
    }

    std::set<std::string> names = subsystems();
    if(names.empty()){
        return results;
    }

    //如果子系统在mount的选项中出现,则说明这个子系统是在hierarchy下
	std::set<std::string>::iterator iter,end;
	end = names.end();
	for(iter = names.begin(); iter != end; ++iter){
        const std::string& name = *iter;
        if(hierarchyEntry.hasOption(name)){
            results.insert(name);
        }
    }

    return results;
}

bool busy(const std::string& subsystems)
{
    std::map<std::string,SubsystemInfo> infos = internel::subsystems();
    if(infos.empty()){
        return false;
    }

    bool busy = false;
    std::map<std::string,SubsystemInfo>::iterator end;
    end = infos.end();

    //查找子系统是否在所有子系统列表中出现
    std::vector<std::string> subsystems_items = strings::tokenize(subsystems,",");
    size_t n = subsystems_items.size();
    size_t i = 0;
    for(; i < n; ++i){
        std::string& subsystem = subsystems_items.at(i);
        if(end == infos.find(subsystem)){
            return false;
        }

        if(infos[subsystem].hierarchy != 0){
            busy = true;
        }
    }

    return busy;
}

bool enabled(const std::string& subsystems)
{
    std::map<std::string,SubsystemInfo> infos = internel::subsystems();
    bool disable = false;

    std::vector<std::string> subsystems_items = strings::tokenize(subsystems,",");
    size_t n = subsystems_items.size();
    size_t i = 0;

    for(; i < n; ++i){
        std::string& subsystem = subsystems_items.at(i);
        if(infos.find(subsystem) == infos.end()){
            return false;
        }

        if(!infos[subsystem].enabled){
            disable = true;
        }
    }

    return !disable;
}

int mount(const std::string& hierarchy, const std::string& subsystems, int retry)
{
    int mounted = internel::mount(hierarchy,subsystems);
    if(mounted < 0 && retry > 0){
        usleep(100);
        return cgroups::mount(hierarchy,subsystems,retry - 1);
    }

    return mounted;
}


int unmount(const std::string& hierarchy)
{
    if(!verify(hierarchy)){
        return -1;
    }

    if(internel::unmount(hierarchy) < 0){
        return -1;
    }

    bool ret = os::rmdir(hierarchy);
    return ret ? 0 : -1;
}

int remove(const std::string& hierarchy, const std::string& cgroup)
{
  bool ret = verify(hierarchy, cgroup);
  if (!ret) {
    return -1;
  }

  std::vector<std::string > cgroups = cgroups::get(hierarchy, cgroup);
  if (!cgroups.empty()) {
    return -1;
  }

  return internel::remove(hierarchy, cgroup);
}

std::vector<std::string> get(const std::string& hierarchy, const std::string& cgroup )
{
    std::vector<std::string> cgroups;
    bool error = verify(hierarchy, cgroup);
    if (error) {
        return cgroups;
    }

    std::string hierarchyAbsPath = os::realpath(hierarchy);
    if (hierarchyAbsPath.empty()) {
    return cgroups;
    }

    std::string destAbsPath = os::realpath(path::join(hierarchy, cgroup));
    if (destAbsPath.empty()) {
    return cgroups;
    }

    char* paths[] = { const_cast<char*>(destAbsPath.c_str()), NULL };

    FTS* tree = fts_open(paths, FTS_NOCHDIR, NULL);
    if (tree == NULL) {
    return cgroups;
    }


    //在指定cgroup下是否还有目录,也就是是否还有控制族群
    FTSENT* node;
    while ((node = fts_read(tree)) != NULL) {
    // Use post-order walk here. fts_level is the depth of the traversal,
    // numbered from -1 to N, where the file/dir was found. The traversal root
    // itself is numbered 0. fts_info includes flags for the current node.
    // FTS_DP indicates a directory being visited in postorder.
    if (node->fts_level > 0 && node->fts_info & FTS_DP) {
        //hierarchyAbsPath.get().length()??
      std::string path = strings::trim(node->fts_path + hierarchyAbsPath.length(), "/");
      cgroups.push_back(path);
    }
    }

    if (errno != 0) {
    return cgroups;
    }

    if (fts_close(tree) != 0) {
    return cgroups;
    }

    return cgroups;
}

bool exists(const std::string& hierarchy, const std::string& cgroup,const std::string& control)
{
  bool ret = verify(hierarchy, cgroup);
  if (!ret) {
    return false;
  }

  return os::exists(path::join(hierarchy, cgroup, control));
}

std::string read(const std::string& hierarchy,const std::string& cgroup,const std::string& control)
{
  bool ret = verify(hierarchy, cgroup, control);
  if (!ret) {
    return false;
  }

  return internel::read(hierarchy, cgroup, control);
}

int write(const std::string& hierarchy,const std::string& cgroup,const std::string& control,const std::string& value)
{
  bool ret = verify(hierarchy, cgroup, control);
  if (!ret) {
    return -1;
  }

  return internel::write(hierarchy, cgroup, control, value);
}

std::set<pid_t> tasks(const std::string& hierarchy, const std::string& cgroup)
{
  std::set<pid_t> pids;
  std::string value = cgroups::read(hierarchy, cgroup, "tasks");
  if ("") {
    return pids;
  }

  // Parse the value read from the control file.
  std::istringstream ss(value);
  ss >> std::dec;
  while (!ss.eof()) {
    pid_t pid;
    ss >> pid;

    if (ss.fail()) {
      if (!ss.eof()) {
        return pids;
      }
    } else {
      pids.insert(pid);
    }
  }

  return pids;
}

int assign(const std::string& hierarchy, const std::string& cgroup, pid_t pid)
{
    std::ostringstream oss;
    oss << pid;
    return cgroups::write(hierarchy, cgroup, "tasks", oss.str());
}

bool cleanup(const std::string& hierarchy)
{
  bool mounted = cgroups::mounted(hierarchy);
  if (!mounted) {
    if (os::exists(hierarchy)) {
      bool ret = os::rmdir(hierarchy);
      if (!ret) {
        return false;
      }
    }
    return false;
  }

  if (mounted) {
      // Remove the hierarchy.
      int unmount = cgroups::unmount(hierarchy);
      if (unmount < 0) {
        return false;
      }

      // Remove the directory if it still exists.
      if (os::exists(hierarchy)) {
        bool rmdir = os::rmdir(hierarchy);
        if (!rmdir) {
          return false;
        }
      }
  }

  return true;
}

int create(const std::string& hierarchy, const std::string& cgroup);

} //namespace cgroups

