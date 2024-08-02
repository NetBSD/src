#ifndef _NEWFS_EXFATFS_DEFAULTS_H
#define _NEWFS_EXFATFS_DEFAULTS_H

#include <sys/types.h>

off_t default_cluster_shift(off_t);
off_t default_fatalign(off_t);
off_t default_heapalign(off_t);
struct exfatfs *default_exfat_sb(struct dkwedge_info *);
int default_upcase_table(uint16_t **, size_t *);

#endif /* _NEWFS_EXFATFS_DEFAULTS_H */
