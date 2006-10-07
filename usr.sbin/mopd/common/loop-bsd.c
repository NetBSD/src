/*	$NetBSD: loop-bsd.c,v 1.9 2006/10/07 17:27:57 elad Exp $	*/

/*
 * Copyright (c) 1993-95 Mats O Jansson.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mats O Jansson.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: loop-bsd.c,v 1.9 2006/10/07 17:27:57 elad Exp $");
#endif

#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#if defined(__bsdi__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/time.h>
#endif
#include <net/bpf.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <assert.h>

#include "os.h"
#include "common.h"
#include "device.h"
#include "mopdef.h"
#include "log.h"

int
mopOpenRC(p, trans)
	struct if_info *p;
	int	trans;
{
#ifndef NORC
	return (*(p->iopen))(p->if_name,
			     O_RDWR,
			     MOP_K_PROTO_RC,
			     trans);
#else
	return -1;
#endif
}

int
mopOpenDL(p, trans)
	struct if_info *p;
	int	trans;
{
#ifndef NODL
	return (*(p->iopen))(p->if_name,
			     O_RDWR,
			     MOP_K_PROTO_DL,
			     trans);
#else
	return -1;
#endif
}

void
mopReadRC()
{
}

void
mopReadDL()
{
}

/*
 * The list of all interfaces that are being listened to.  loop()
 * "polls" on the descriptors in this list.
 */
struct if_info *iflist;

void   mopProcess    __P((struct if_info *, u_char *));

/*
 * Loop indefinitely listening for MOP requests on the
 * interfaces in 'iflist'.
 */
void
Loop()
{
	u_char *buf, *bp, *ep;
	int     cc, n, m;
	struct	pollfd *set;
	int     bufsize;
	struct	if_info *ii;

	if (iflist == 0)
		mopLogErrX("no interfaces");
	if (iflist->fd != -1) {
		if (ioctl(iflist->fd, BIOCGBLEN, (caddr_t) & bufsize) < 0)
			mopLogErr("BIOCGBLEN");
	} else
		mopLogErrX("cannot get buffer size");
	buf = (u_char *) malloc((unsigned) bufsize);
	if (buf == 0)
		mopLogErr("malloc");
	/*
         * Find the highest numbered file descriptor for poll().
         * Initialize the set of descriptors to listen to.
         */
	for (ii = iflist, n = 0; ii; ii = ii->next, n++)
		;
	set = malloc(n * sizeof(*set));
	for (ii = iflist, m = 0; ii; ii = ii->next, m++) {
		assert(ii->fd != -1);
		set[m].fd = ii->fd;
		set[m].events = POLLIN;
	}
	for (;;) {
		if (poll(set, n, INFTIM) < 0)
			mopLogErr("poll");
		for (ii = iflist, m = 0; ii; ii = ii->next, m++) {
			if (!(set[m].revents & POLLIN))
				continue;
	again:
			cc = read(ii->fd, (char *) buf, bufsize);
			/* Don't choke when we get ptraced */
			if (cc < 0 && errno == EINTR)
				goto again;
			/* Due to a SunOS bug, after 2^31 bytes, the file
			 * offset overflows and read fails with EINVAL.  The
			 * lseek() to 0 will fix things. */
			if (cc < 0) {
				if (errno == EINVAL &&
				    (lseek(ii->fd, 0, SEEK_CUR) + bufsize) < 0) {
					(void) lseek(ii->fd, 0, 0);
					goto again;
				}
				mopLogErr("read");
			}
			/* Loop through the packet(s) */
#define bhp ((struct bpf_hdr *)bp)
			bp = buf;
			ep = bp + cc;
			while (bp < ep) {
				int caplen, hdrlen;

				caplen = bhp->bh_caplen;
				hdrlen = bhp->bh_hdrlen;
				mopProcess(ii, bp + hdrlen);
				bp += BPF_WORDALIGN(hdrlen + caplen);
			}
		}
	}
}
