#include <fstab.h>
#include <mntent.h>

#include <sys/mount.h>

#include <string>
#include <vector>

// Structure describing a mount table (e.g. /etc/mtab or /proc/mounts).
struct MountTable {
  // Structure describing a mount table entry. This is a wrapper for struct
  // mntent defined in <mntent.h>.
  struct Entry {
    Entry() : freq(0), passno(0) {}

    Entry(const std::string& _fsname,
          const std::string& _dir,
          const std::string& _type,
          const std::string& _opts,
          int _freq,
          int _passno)
      : fsname(_fsname),
        dir(_dir),
        type(_type),
        opts(_opts),
        freq(_freq),
        passno(_passno)
    {}

    // Check whether a given mount option exists in a mount table entry.
    // @param   option    The given mount option.
    // @return  Whether the given mount option exists.
    bool hasOption(const std::string& option);

    std::string fsname; // Device or server for filesystem.
    std::string dir;    // Directory mounted on.
    std::string type;   // Type of filesystem: ufs, nfs, etc.
    std::string opts;   // Comma-separated options for fs.
    int freq;           // Dump frequency (in days).
    int passno;         // Pass number for `fsck'.
  };

  // Read the mount table from a file.
  // @param   path    The path of the file storing the mount table.
  // @return  An instance of MountTable if success.
  static MountTable* read(const std::string& path);

  std::vector<Entry> entries;
};

struct SubsystemInfo
{
  SubsystemInfo()
    : hierarchy(0),
      cgroups(0),
      enabled(false) {}

  SubsystemInfo(const std::string& _name,
                int _hierarchy,
                int _cgroups,
                bool _enabled)
    : name(_name),
      hierarchy(_hierarchy),
      cgroups(_cgroups),
      enabled(_enabled) {}

  std::string name;      // Name of the subsystem.
  int hierarchy;    // ID of the hierarchy the subsystem is attached to.
  int cgroups;      // Number of cgroups for the subsystem.
  bool enabled;     // Whether the subsystem is enabled or not.
};