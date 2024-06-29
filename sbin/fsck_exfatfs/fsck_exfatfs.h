#ifndef FSCK_EXFATFS_H_
#define FSCK_EXFATFS_H_

#include <sys/queue.h>

struct ino_entry {
	LIST_ENTRY(ino_entry) list;
	ino_t ino;
};

extern int bitmap_discontiguous;
extern int problems;
extern int debug;
extern int Nflag;
extern int Pflag;
extern int Qflag;
extern int Yflag;

int reply(const char * /* question */);

#endif /* FSCK_EXFATFS_H_ */
