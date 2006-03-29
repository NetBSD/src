/* dirent.h for vms */

#ifndef VMSDIR_H
#define VMSDIR_H

#include <rms.h>

#define	MAXNAMLEN	255

#ifndef __DECC
#if !defined (__GNUC__) && !defined (__ALPHA)
typedef unsigned long u_long;
typedef unsigned short u_short;
#endif
#endif

struct direct
{
  off_t d_off;
  u_long d_fileno;
  u_short d_reclen;
  u_short d_namlen;
  char d_name[MAXNAMLEN + 1];
};

#undef DIRSIZ
#define DIRSIZ(dp)		\
  (((sizeof (struct direct)	\
     - (MAXNAMLEN+1)		\
     + ((dp)->d_namlen+1))	\
    + 3) & ~3)

#define d_ino	d_fileno		/* compatability */


/*
 * Definitions for library routines operating on directories.
 */

typedef struct DIR
{
  struct direct dir;
  char d_result[MAXNAMLEN + 1];
#if defined (__ALPHA) || defined (__DECC)
  struct FAB fab;
#else
  struct fabdef fab;
#endif
} DIR;

#ifndef NULL
#define NULL 0
#endif

extern	DIR *opendir PARAMS (());
extern	struct direct *readdir PARAMS ((DIR *dfd));
#define rewinddir(dirp)	seekdir((dirp), (long)0)
extern	int closedir PARAMS ((DIR *dfd));
extern char *vmsify PARAMS ((char *name, int type));

#endif /* VMSDIR_H */
