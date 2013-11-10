#ifdef _TEST_SET_
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

using namespace std;

std::string read(const std::string& path)
{
  std::string content;
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
  content = ss.str();
  return content;
}

int write(const std::string& path,const string& value)
{
  std::ofstream file(path.c_str());

  if (!file.is_open()) {
    return -1;
  }

  file << value << std::endl;

  if (file.fail()) {
    std::cerr << "file write error:" << strerror(errno) << std::endl;
    file.close();
    return -1;
  }

  file.close();
  return 0;
}

int write_ansi(const std::string& path,const string& value)
{
  FILE* file = fopen(path.c_str(),"we");
  int nret = fwrite(value.c_str(),value.size(),1,file);
  
  if(ferror(file)){
	std::cout << "file error " << strerror(errno) << "\n";
  }

  fclose(file);
  return 0;
}

int main()
{
  std::string path = "/cgroup/memory/submem/memory.limit_in_bytes";
  std::string value= "104857600";
  int ret = 0;

  std::string bvalue1 = read(path);
  ret = write(path,value);
  std::cout << "write ret:" << ret << std::endl;
  std::string bvalue2 = read(path);
  ret = write_ansi(path,value);
  std::cout << "write ansi ret:" << ret << std::endl;
  std::string bvalue3 = read(path);

  std::cout << "original bvalue1 " << bvalue1 
          << "fstream  bvalue2 " << bvalue2 
          << "asni bvalue3 " << bvalue3 
          << std::endl;

  return 0;
}

#endif
