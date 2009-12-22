/*	$NetBSD: umserv.c,v 1.1 2009/12/22 18:36:02 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This is a [quick, simple & dirty] userspace sd@umass server.
 * We probe USB devices using rump and attach them to the host kernel
 * using pud(4).  The resulting block devices can be e.g. read
 * and/or mounted.
 *
 * Since there is no devfs support in NetBSD, we create crudo & cotto
 * device nodes in the current directory.  Operating on these in the
 * host OS will direct operations to this userspace server, e.g.:
 *   golem> disklabel ./rumpsd0d
 *   golem> mount_msdos ./rumpsd0e /mnt
 * will cause file system access to /mnt be backed by the umass server
 * in userspace.  Due to the relatively experimental nature of this
 * server, rump file servers are recommended for mounting experiments.
 */

#include <sys/types.h>
#include <sys/syslimits.h>

#include <dev/pud/pud_msgif.h>

#include <rump/rump.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * No devfs?  No problem.  We just hack a bit & wait for the dust to settle.
 */
#define NODEBASE "rumpsd0"
#define MYMAJOR 411
static int
makenodes(void)
{
	char path[PATH_MAX];
	struct stat sb;
	int i, j, rv;

	for (i = 0; i < 2; i++) {
		int minnum = 0;

		for (j = 0; j < 8; j++, minnum++) {
			sprintf(path, "%s%s%c",
			    i == 0 ? "" : "r", NODEBASE, minnum + 'a');
			if (stat(path, &sb) == 0)
				continue;
			rv = mknod(path, (i == 0 ? S_IFBLK : S_IFCHR) | 0666,
			    makedev(MYMAJOR, minnum));
			if (rv != 0 && !(rv == -1 && errno == EEXIST))
				return rv;
		}
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	char path[PATH_MAX];
	struct pud_conf_reg pcr;
	struct pud_req *pdr;
	struct vnode *devvps[8], *devvp;
	kauth_cred_t rootcred;
	ssize_t n;
	int fd, rv, i;

	if (makenodes() == -1)
		err(1, "makenodes");

	fd = open(_PATH_PUD, O_RDWR);
	if (fd == -1)
		err(1, "open");

#define PDRSIZE (64*1024+1024)
	pdr = malloc(PDRSIZE);
	if (pdr == NULL)
		err(1, "malloc");

	memset(&pcr, 0, sizeof(pcr));
	pcr.pm_pdr.pdr_pth.pth_framelen = sizeof(struct pud_conf_reg);
	pcr.pm_version = PUD_DEVELVERSION | PUD_VERSION;
	pcr.pm_pdr.pdr_reqclass = PUD_REQ_CONF;
	pcr.pm_pdr.pdr_reqtype = PUD_CONF_REG;

	pcr.pm_regdev = makedev(MYMAJOR, 0);
	pcr.pm_flags = PUD_CONFFLAG_BDEV;
	strlcpy(pcr.pm_devname, "youmass", sizeof(pcr.pm_devname));

	n = write(fd, &pcr, pcr.pm_pdr.pdr_pth.pth_framelen);
	if (n == -1)
		err(1, "configure"); /* XXX: doubles as protocol error */

	rump_boot_sethowto(RUMP_AB_VERBOSE);
	rump_init();

	/*
	 * We use the raw devices.  This avoids undesired block caching
	 * in read/write.  It's also simpler.  It's even mostly correct.
	 *
	 * As is probably obvious by now, we execute ops through specfs.
	 * Why?  Can't say it wasn't because specfs-via-vnodes was
	 * already supported by rump.  But, that's mostly how the
	 * kernel does it (cf. mounting file system etc).
	 */
	for (i = 0; i < 8; i++) {
		sprintf(path, "/dev/rsd0%c", 'a' + i);
		if ((rv = rump_pub_namei(RUMP_NAMEI_LOOKUP, 0, path,
		    NULL, &devvps[i], NULL)) != 0)
			errx(1, "raw device lookup failed %d", rv);
	}

	rootcred = rump_pub_cred_create(0, 0, 0, NULL);

	/* process requests ad infinitum */
	for (;;) {
		struct pud_creq_open *pr_open;
		struct pud_creq_close *pr_close;
		struct pud_req_readwrite *pr_rw;
		struct pud_req_ioctl *pr_ioctl;
		struct uio *uio;
		size_t reslen;
		int minordev;

		n = read(fd, pdr, PDRSIZE);
		if (n == -1)
			err(1, "read");

		minordev = minor(pdr->pdr_dev);
		if (minordev < 0 || minordev >= 8) {
			rv = ENXIO;
			goto sendresponse;
		}
		devvp = devvps[minordev];

		switch (pdr->pdr_reqtype) {
		case PUD_BDEV_OPEN:
			pr_open = (void *)pdr;
			RUMP_VOP_LOCK(devvp, RUMP_LK_EXCLUSIVE);
			rv = RUMP_VOP_OPEN(devvp, pr_open->pm_fmt, rootcred);
			RUMP_VOP_UNLOCK(devvp, 0);
			break;

		case PUD_BDEV_CLOSE:
			pr_close = (void *)pdr;
			RUMP_VOP_LOCK(devvp, RUMP_LK_EXCLUSIVE);
			rv = RUMP_VOP_CLOSE(devvp, pr_close->pm_fmt, rootcred);
			RUMP_VOP_UNLOCK(devvp, 0);
			break;

		case PUD_BDEV_IOCTL:
			pr_ioctl = (void *)pdr;
			rv = RUMP_VOP_IOCTL(devvp, pr_ioctl->pm_iocmd,
			    pr_ioctl->pm_data, pr_ioctl->pm_flag, rootcred);
			break;

		case PUD_BDEV_STRATREAD:
			pr_rw = (void *)pdr;
			assert(pr_rw->pm_resid <= 64*1024);
			uio = rump_pub_uio_setup(&pr_rw->pm_data[0],
			    pr_rw->pm_resid, pr_rw->pm_offset, RUMPUIO_READ);
			RUMP_VOP_LOCK(devvp, RUMP_LK_SHARED);
			rv = RUMP_VOP_READ(devvp, uio, 0, rootcred);
			RUMP_VOP_UNLOCK(devvp, 0);
			reslen = rump_pub_uio_free(uio);
			pdr->pdr_pth.pth_framelen -= reslen;
			pr_rw->pm_resid = reslen;
			break;

		case PUD_BDEV_STRATWRITE:
			pr_rw = (void *)pdr;
			uio = rump_pub_uio_setup(&pr_rw->pm_data[0],
			    pr_rw->pm_resid, pr_rw->pm_offset, RUMPUIO_WRITE);
			RUMP_VOP_LOCK(devvp, RUMP_LK_EXCLUSIVE);
			rv = RUMP_VOP_WRITE(devvp, uio, 0, rootcred);
			RUMP_VOP_UNLOCK(devvp, 0);
			reslen = rump_pub_uio_free(uio);
			pr_rw->pm_resid = reslen;
			pdr->pdr_pth.pth_framelen=sizeof(struct pud_creq_write);
			rv = 0;
			break;

		}

 sendresponse:
		pdr->pdr_rv = rv;
		n = write(fd, pdr, pdr->pdr_pth.pth_framelen);
		assert(n == (ssize_t)pdr->pdr_pth.pth_framelen);
	}
}
