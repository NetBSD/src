/*	$NetBSD: conf.c,v 1.2 2005/06/23 19:44:01 junyoung Exp $	*/

#include <stand.h>
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
