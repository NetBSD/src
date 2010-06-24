/*	$NetBSD: ttyserv.c,v 1.2 2010/06/24 13:03:05 hannken Exp $	*/

/*
 * Copyright (c) 2009, 2010 Antti Kantee.  All Rights Reserved.
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
 * This is a [quick, simple & dirty] userspace tty/ucom server.
 * We probe USB devices using rump and attach them to the host kernel
 * using pud(4).
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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * No devfs?  No problem.  We just hack a bit & wait for the dust to settle.
 */
#define MYMAJOR 412
static int
makenodes(void)
{
	struct stat sb;
	int rv;

	if (stat("rumpttyU0", &sb) != 0)
		rv = mknod("rumpttyU0", S_IFCHR | 0666, makedev(MYMAJOR, 0));
	if (rv != 0 && !(rv == -1 && errno == EEXIST))
		return rv;

	if (stat("rumpttyU1", &sb) != 0)
		rv = mknod("rumpttyU1", S_IFCHR | 0666, makedev(MYMAJOR, 1));
	if (rv != 0 && !(rv == -1 && errno == EEXIST))
		return rv;

	return 0;
}

static struct vnode *devvps[2];
static kauth_cred_t rootcred;
static int fd;

static void *
handlereq(void *arg)
{
	struct vnode *devvp;
	struct pud_creq_open *pr_open;
	struct pud_creq_close *pr_close;
	struct pud_req_readwrite *pr_rw;
	struct pud_req_ioctl *pr_ioctl;
	struct uio *uio;
	size_t reslen;
	struct pud_req *pdr = arg;
	int minordev, rv;
	ssize_t n;

	minordev = minor(pdr->pdr_dev);
	if (minordev < 0 || minordev >= 8) {
		rv = ENXIO;
		goto sendresponse;
	}
	devvp = devvps[minordev];

	switch (pdr->pdr_reqtype) {
	case PUD_CDEV_OPEN:
		pr_open = (void *)pdr;
		RUMP_VOP_LOCK(devvp, RUMP_LK_EXCLUSIVE);
		rv = RUMP_VOP_OPEN(devvp, pr_open->pm_fmt, rootcred);
		RUMP_VOP_UNLOCK(devvp);
		break;

	case PUD_CDEV_CLOSE:
		pr_close = (void *)pdr;
		RUMP_VOP_LOCK(devvp, RUMP_LK_EXCLUSIVE);
		rv = RUMP_VOP_CLOSE(devvp, pr_close->pm_fmt, rootcred);
		RUMP_VOP_UNLOCK(devvp);
		break;

	case PUD_CDEV_IOCTL:
		pr_ioctl = (void *)pdr;
		rv = RUMP_VOP_IOCTL(devvp, pr_ioctl->pm_iocmd,
		    pr_ioctl->pm_data, pr_ioctl->pm_flag, rootcred);
		break;

	case PUD_CDEV_READ:
		pr_rw = (void *)pdr;
		assert(pr_rw->pm_resid <= 64*1024);
		uio = rump_pub_uio_setup(&pr_rw->pm_data[0],
		    pr_rw->pm_resid, pr_rw->pm_offset, RUMPUIO_READ);
		RUMP_VOP_LOCK(devvp, RUMP_LK_SHARED);
		rv = RUMP_VOP_READ(devvp, uio, 0, rootcred);
		RUMP_VOP_UNLOCK(devvp);
		reslen = rump_pub_uio_free(uio);
		pdr->pdr_pth.pth_framelen -= reslen;
		pr_rw->pm_resid = reslen;
		break;

	case PUD_CDEV_WRITE:
		pr_rw = (void *)pdr;
		uio = rump_pub_uio_setup(&pr_rw->pm_data[0],
		    pr_rw->pm_resid, pr_rw->pm_offset, RUMPUIO_WRITE);
		RUMP_VOP_LOCK(devvp, RUMP_LK_EXCLUSIVE);
		rv = RUMP_VOP_WRITE(devvp, uio, 0, rootcred);
		RUMP_VOP_UNLOCK(devvp);
		reslen = rump_pub_uio_free(uio);
		pr_rw->pm_resid = reslen;
		pdr->pdr_pth.pth_framelen=sizeof(struct pud_creq_write);
		rv = 0;
		break;
	default:
		printf("unknown request %d\n", pdr->pdr_reqtype);
		abort();
	}

sendresponse:
	printf("result %d\n", rv);
	pdr->pdr_rv = rv;
	n = write(fd, pdr, pdr->pdr_pth.pth_framelen);
	assert(n == (ssize_t)pdr->pdr_pth.pth_framelen);

	pthread_exit(NULL);
}

int
main(int argc, char *argv[])
{
	struct pud_conf_reg pcr;
	struct pud_req *pdr;
	ssize_t n;
	int rv;

	if (makenodes() == -1)
		err(1, "makenodes");

	fd = open(_PATH_PUD, O_RDWR);
	if (fd == -1)
		err(1, "open");

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

	if ((rv = rump_pub_namei(RUMP_NAMEI_LOOKUP, 0, "/dev/ttyU0",
	    NULL, &devvps[0], NULL)) != 0)
		errx(1, "raw device 0 lookup failed %d", rv);
	if ((rv = rump_pub_namei(RUMP_NAMEI_LOOKUP, 0, "/dev/ttyU1",
	    NULL, &devvps[1], NULL)) != 0)
		errx(1, "raw device 1 lookup failed %d", rv);

	rootcred = rump_pub_cred_create(0, 0, 0, NULL);

	/* process requests ad infinitum */
	for (;;) {
		pthread_t pt;

#define PDRSIZE (64*1024+1024)
		pdr = malloc(PDRSIZE);
		if (pdr == NULL)
			err(1, "malloc");

		n = read(fd, pdr, PDRSIZE);
		if (n == -1)
			err(1, "read");

		/*
		 * tip & cu fork and read/write at the same time.  hence,
		 * we need a multithreaded server as otherwise read requests
		 * will block eternally since no writes can be done.
		 *
		 * XXX: do this properly (detached threads, or pool)
		 */
		pthread_create(&pt, NULL, handlereq, pdr);
	}
}
