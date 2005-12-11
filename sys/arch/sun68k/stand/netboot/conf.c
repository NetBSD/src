/*	$NetBSD: conf.c,v 1.3 2005/12/11 12:19:29 christos Exp $	*/

#include <sys/types.h>
#include <netinet/in.h>

#include "stand.h"
#include "nfs.h"
#include "dev_net.h"

struct fs_ops file_system[] = {
	FS_OPS(nfs),
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int ndevs = 1;

int
main() {
	xxboot_main("netboot");
}
