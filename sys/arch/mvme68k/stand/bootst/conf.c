/*	$NetBSD: conf.c,v 1.1.68.1 2005/11/10 13:57:53 skrll Exp $	*/

#include <lib/libsa/stand.h>
#include <rawfs.h>
#include <dev_tape.h>

struct fs_ops file_system[] = {
	FS_OPS(rawfs),
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "tape", tape_strategy, tape_open, tape_close, tape_ioctl },
};
int ndevs = 1;

int debug;
