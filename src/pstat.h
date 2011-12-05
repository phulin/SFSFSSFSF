#include <sys/stat.h>
#include <stdint.h>

// TODO: Implement endianeness compatibility

// Portable stat struct, although we don't care about the filesystem
// and device-specific stuff. We also don't keep uid/gid, as that doesn't
// make sense across machines.

struct pstat {
	uint64_t pst_mode;    /* protection */
	uint64_t pst_size;    /* total size, in bytes */
	uint64_t pst_atime;   /* time of last access */
	uint64_t pst_mtime;   /* time of last modification */
	uint64_t pst_ctime;   /* time of last status change */
};

void pstat_write(struct stat *s, struct pstat *ps);
void pstat_read(struct pstat *ps, struct stat *s);
