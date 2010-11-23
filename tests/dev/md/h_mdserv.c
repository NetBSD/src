/*	$NetBSD: h_mdserv.c,v 1.1 2010/11/23 15:38:54 pooka Exp $	*/

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <dev/md.h>

#include <fcntl.h>
#include <unistd.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "../../h_macros.h"

#define MDSIZE (1024*1024)

int
main(void)
{
	struct md_conf md;
	int fd;

	md.md_addr = malloc(MDSIZE);
	md.md_size = MDSIZE;
	md.md_type = MD_UMEM_SERVER;

	RL(rump_init());
	RL(fd = rump_sys_open("/dev/rmd0d", O_RDWR));
	RL(rump_sys_ioctl(fd, MD_SETCONF, &md));
	pause();
}
