#ifndef BOOT_H_
#define BOOT_H_

#include <sys/types.h>

typedef void (*boot_entry_t)(int, int, int (*)(void *), void *, u_int);

void main(void);

#define MAXBOOTPATHLEN	256
extern char bootdev[MAXBOOTPATHLEN];
extern bool floppyboot;

#ifdef HAVE_CHANGEDISK_HOOK
struct open_file;

void changedisk_hook(struct open_file *of);
#endif

#endif /* BOOT_H_ */
