/* $NetBSD: devopen.c,v 1.1.2.1 2007/06/02 08:44:36 nisimura Exp $ */

#include <sys/param.h>

#include <netinet/in.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/nfs.h>

int net_open(struct open_file *, ...);
int net_close(struct open_file *);
int net_strategy(void *, int, daddr_t, size_t, void *, size_t *);

struct devsw devsw[] = {
	{ "fxp", net_strategy, net_open, net_close, noioctl },
};
int ndevs = sizeof(devsw) / sizeof(devsw[0]);

struct fs_ops ops_nfs = FS_OPS(nfs);
struct fs_ops file_system[1];
int nfsys = 1;

int
devopen(of, name, file)
	struct open_file *of;
	const char *name;
	char **file;
{
	int error;
	extern char bootfile[]; /* handed by DHCP */

	if (of->f_flags != F_READ)
		return EPERM;

	if (strcmp("net:", name) == 0) {
		of->f_dev = &devsw[0];
		if ((error = net_open(of, name)) != 0)
			return error;
		file_system[0] = ops_nfs;
		*file = bootfile;	/* resolved fname */
		return 0;		/* NFS */
	}
	return ENOENT;
}

/* ARGSUSED */
int
noioctl(f, cmd, data)
	struct open_file *f;
	u_long cmd;
	void *data;
{
	return EINVAL;
}
