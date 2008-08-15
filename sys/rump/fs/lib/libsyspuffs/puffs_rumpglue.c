/*	$NetBSD: puffs_rumpglue.c,v 1.5 2008/08/15 15:02:28 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Research Foundation of Helsinki University of Technology
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_rumpglue.c,v 1.5 2008/08/15 15:02:28 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kthread.h>
#include <sys/mount.h>

#include <dev/putter/putter.h>
#include <dev/putter/putter_sys.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

#include "puffs_rumpglue.h"

void putterattach(void); /* XXX: from autoconf */
dev_type_open(puttercdopen);

struct ptargs {
	int comfd;
	int fpfd;
	struct filedesc *fdp;
};

#define BUFSIZE (64*1024)
extern int hz;

/*
 * Read requests from /dev/puffs and forward them to comfd
 *
 * XXX: the init detection is really sucky, but let's not
 * waste too much energy for a better one here
 */
static void
readthread(void *arg)
{
	struct ptargs *pap = arg;
	struct file *fp;
	register_t rv;
	char *buf;
	off_t off;
	int error, inited;

	buf = kmem_alloc(BUFSIZE, KM_SLEEP);
	inited = 0;

 retry:
	kpause(NULL, 0, hz/4, NULL);

	for (;;) {
		ssize_t n;

		off = 0;
		fp = fd_getfile(pap->fpfd);
		error = dofileread(pap->fpfd, fp, buf, BUFSIZE,
		    &off, 0, &rv);
		if (error) {
			if (error == ENOENT && inited == 0)
				goto retry;
			if (error == ENXIO)
				break;
			panic("fileread failed: %d", error);
		}
		inited = 1;

		while (rv) {
			n = rumpuser_write(pap->comfd, buf, rv, &error);
			if (n == -1)
				panic("fileread failed: %d", error);
			if (n == 0)
				panic("fileread failed: closed");
			rv -= n;
		}
	}

	kthread_exit(0);
}

/* Read requests from comfd and proxy them to /dev/puffs */
static void
writethread(void *arg)
{
	struct ptargs *pap = arg;
	struct file *fp;
	struct putter_hdr *phdr;
	register_t rv;
	char *buf;
	uint64_t roff, toread;
	off_t woff;
	int error;

	buf = kmem_alloc(BUFSIZE, KM_SLEEP);
	phdr = (struct putter_hdr *)buf;

	for (;;) {
		ssize_t n;

		/*
		 * Need to write everything to the "kernel" in one chunk,
		 * so make sure we have it here.
		 */
		roff = 0;
		toread = sizeof(struct putter_hdr);
		do {
			n = rumpuser_read(pap->comfd, buf+roff, toread, &error);
			if (n <= 0) {
				if (n == 0)
					break;
				panic("rumpuser_read %zd %d", n, error);
			}
			roff += n;
			if (roff >= sizeof(struct putter_hdr))
				toread = phdr->pth_framelen - roff;
			else
				toread = roff - sizeof(struct putter_hdr);
		} while (toread);

		rv = woff = 0;
		fp = fd_getfile(pap->fpfd);
		error = dofilewrite(pap->fpfd, fp, buf, phdr->pth_framelen,
		    &woff, 0, &rv);
		if (error == ENXIO)
			break;
		KASSERT(rv == phdr->pth_framelen);
	}

	kthread_exit(0);
}

int
puffs_rumpglue_init(int fd, int *newfd)
{
	struct ptargs *pap;
	int rv;

	rump_init();
	putterattach();
	rv = puttercdopen(makedev(178, 0), 0, 0, curlwp);
	if (rv && rv != EMOVEFD)
		return rv;

	pap = kmem_alloc(sizeof(struct ptargs), KM_SLEEP);
	pap->comfd = fd;
	pap->fpfd = curlwp->l_dupfd;
	pap->fdp = curlwp->l_proc->p_fd;

	kthread_create(PRI_NONE, 0, NULL, readthread, pap, NULL, "rputter");
	kthread_create(PRI_NONE, 0, NULL, writethread, pap, NULL, "wputter");

	*newfd = curlwp->l_dupfd;
	return 0;
}
