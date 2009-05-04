/*	$NetBSD: conf.c,v 1.3.78.1 2009/05/04 08:12:02 yamt Exp $	*/

#include <sys/types.h>
#include <netinet/in.h>

#include "stand.h"
#include "nfs.h"
#include "dev_net.h"
#include "libsa.h"

struct fs_ops file_system[] = {
	FS_OPS(nfs),
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int ndevs = 1;

int
main(void)
{

	xxboot_main("netboot");

	return 0;
}
