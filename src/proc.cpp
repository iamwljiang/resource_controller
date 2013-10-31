#include "proc.h"
#include <limits.h>

bool MountTable::Entry::hasOption(const std::string& option)
{
  struct mntent mntent;
  mntent.mnt_fsname = const_cast<char*>(fsname.c_str());
  mntent.mnt_dir = const_cast<char*>(dir.c_str());
  mntent.mnt_type = const_cast<char*>(type.c_str());
  mntent.mnt_opts = const_cast<char*>(opts.c_str());
  mntent.mnt_freq = freq;
  mntent.mnt_passno = passno;
  return ::hasmntopt(&mntent, option.c_str()) != NULL;
}

MountTable* MountTable::read(const std::string& path)
{
    FILE* file = ::setmntent(path.c_str(), "r");
    if(NULL == file) return NULL;

    MountTable* table = new MountTable;
#if defined(_BSD_SOURCE || _SVID_SOURCE)
    while(true){
        char str[PATH_MAX] = "";
        struct mntent mntbuf;
        struct mntent *mnt = ::getmntent_r(file,&mntbuf,str,PATH_MAX);
        if(NULL == mnt)
            break;

        MountTable::Entry entry(mntent->mnt_fsname,
                                mntent->mnt_dir,
                                mntent->mnt_type,
                                mntent->mnt_opts,
                                mntent->mnt_freq,
                                mntent->mnt_passno);
        table->entries.push_back(entry);
    }
#else
    return NULL;
#endif

    ::entmntent(file);
    return table;
}