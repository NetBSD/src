/*	$NetBSD: subr_devsw.c,v 1.1.2.1 2002/05/16 03:39:13 gehenna Exp $	*/
/*-
 * Copyright (c) 2001,2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by MAEKAWA Masahide <gehenna@NetBSD.org>.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * New device switch framework is developing.
 * So debug options are always turned on.
 */
#ifndef DEVSW_DEBUG
#define	DEVSW_DEBUG
#endif /* DEVSW_DEBUG */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#ifdef DEVSW_DEBUG
#define	DPRINTF(x)	printf x
#else /* DEVSW_DEBUG */
#define	DPRINTF(x)
#endif /* DEVSW_DEBUG */

#define	MAXDEVSW	4096	/* the maximum of major device number */
#define	BDEVSW_SIZE	(sizeof(struct bdevsw *))
#define	CDEVSW_SIZE	(sizeof(struct cdevsw *))
#define	DEVSWCONV_SIZE	(sizeof(struct devsw_conv))

extern const struct bdevsw **bdevsw, *bdevsw0[];
extern const struct cdevsw **cdevsw, *cdevsw0[];
extern struct devsw_conv *devsw_conv, devsw_conv0[];
extern int nbdevsw, ncdevsw, devsw_nconvs;

int
devsw_attach(const char *devname, const struct bdevsw *bdev, int *bmajor,
	     const struct cdevsw *cdev, int *cmajor)
{
	int i;

	if (bdev == NULL && cdev == NULL)
		return (EINVAL);
	if (devname == NULL && bdev != NULL)
		return (EINVAL);

	if (bdevsw_lookup_major(bdev) != -1 ||
	    cdevsw_lookup_major(cdev) != -1)
		return (EEXIST);

	if (bdev != NULL) {
		/* XXX */
		if (*bmajor < 0) {
			for (i = nbdevsw ; i < MAXDEVSW ; i++) {
				if (bdevsw[i] != NULL)
					continue;
				*bmajor = i;
				break;
			}
			if (*bmajor < 0 || *bmajor >= MAXDEVSW)
				return (EINVAL);
		}

		if (*bmajor >= nbdevsw) {
			int old = nbdevsw, new = *bmajor;
			void **newptr;

			newptr = malloc(new * BDEVSW_SIZE, M_DEVBUF, M_NOWAIT);
			if (newptr == NULL)
				return (ENOMEM);
			memset(newptr + old, 0, (new - old) * BDEVSW_SIZE);
			if (old != 0) {
				memcpy(newptr, bdevsw, old * BDEVSW_SIZE);
				if (bdevsw != bdevsw0)
					free(bdevsw, M_DEVBUF);
			}
			bdevsw = (const struct bdevsw **)newptr;
			nbdevsw = new;
		}

		if (bdevsw[*bmajor] != NULL)
			return (EEXIST);
	}

	if (cdev != NULL) {
		/* XXX */
		if (*cmajor < 0) {
			for (i = ncdevsw ; i < MAXDEVSW ; i++) {
				if (cdevsw[i] != NULL)
					continue;
				*cmajor = i;
				break;
			}
			if (*cmajor < 0 || *cmajor >= MAXDEVSW)
				return (EINVAL);
		}

		if (*cmajor >= ncdevsw) {
			int old = ncdevsw, new = *cmajor;
			void **newptr;

			newptr = malloc(new * CDEVSW_SIZE, M_DEVBUF, M_NOWAIT);
			if (newptr == NULL)
				return (ENOMEM);
			memset(newptr + old, 0, (new - old) * CDEVSW_SIZE);
			if (old != 0) {
				memcpy(newptr, cdevsw, old * CDEVSW_SIZE);
				if (cdevsw != cdevsw0)
					free(cdevsw, M_DEVBUF);
			}
			cdevsw = (const struct cdevsw **)newptr;
			ncdevsw = new;
		}

		if (cdevsw[*cmajor] != NULL)
			return (EEXIST);
	}

	if (devname != NULL) {
		for (i = 0 ; i < devsw_nconvs ; i++) {
			if (devsw_conv[i].d_name == NULL)
				break;
		}
		if (i == devsw_nconvs) {
			int old = devsw_nconvs, new = old + 1;
			void *newptr;

			newptr = malloc(new * DEVSWCONV_SIZE,
					M_DEVBUF, M_NOWAIT);
			if (newptr == NULL)
				return (ENOMEM);
			if (old != 0) {
				memcpy(newptr, devsw_conv,
					old * DEVSWCONV_SIZE);
				if (devsw_conv != devsw_conv0)
					free(devsw_conv, M_DEVBUF);
			}
			devsw_conv = (struct devsw_conv *)newptr;
			devsw_nconvs = new;
		}
	}

	if (bdev != NULL)
		bdevsw[*bmajor] = bdev;
	if (cdev != NULL)
		cdevsw[*cmajor] = cdev;
	if (devname != NULL) {
		devsw_conv[i].d_name = devname;
		devsw_conv[i].d_bmajor = *bmajor;
		devsw_conv[i].d_cmajor = *cmajor;
	}

	return (0);
}

void
devsw_detach(const struct bdevsw *bdev, const struct cdevsw *cdev)
{
	int bmajor, i;

	bmajor = -1;
	if (bdev != NULL) {
		for (i = 0 ; i < nbdevsw ; i++) {
			if (bdevsw[i] != bdev)
				continue;
			bdevsw[i] = NULL;
			bmajor = i;
			break;
		}
	}
	if (cdev != NULL) {
		for (i = 0 ; i < ncdevsw ; i++) {
			if (cdevsw[i] != cdev)
				continue;
			cdevsw[i] = NULL;
			break;
		}
	}

	if (bmajor != -1) {
		for (i = 0 ; i < devsw_nconvs ; i++) {
			if (devsw_conv[i].d_bmajor != bmajor)
				continue;

			devsw_conv[i].d_name = NULL;
			devsw_conv[i].d_bmajor = -1;
			devsw_conv[i].d_cmajor = -1;
			break;
		}
	}
}

const struct bdevsw *
bdevsw_lookup(dev_t dev)
{
	int bmajor;

	if (dev == NODEV)
		return (NULL);
	bmajor = major(dev);
	if (bmajor < 0 || bmajor >= nbdevsw)
		return (NULL);

	return (bdevsw[bmajor]);
}

const struct cdevsw *
cdevsw_lookup(dev_t dev)
{
	int cmajor;

	if (dev == NODEV)
		return (NULL);
	cmajor = major(dev);
	if (cmajor < 0 || cmajor >= ncdevsw)
		return (NULL);

	return (cdevsw[cmajor]);
}

int
bdevsw_lookup_major(const struct bdevsw *bdev)
{
	int bmajor;

	for (bmajor = 0 ; bmajor < nbdevsw ; bmajor++) {
		if (bdevsw[bmajor] == bdev)
			return (bmajor);
	}

	return (-1);
}

int
cdevsw_lookup_major(const struct cdevsw *cdev)
{
	int cmajor;

	for (cmajor = 0 ; cmajor < ncdevsw ; cmajor++) {
		if (cdevsw[cmajor] == cdev)
			return (cmajor);
	}

	return (-1);
}

/*
 * Convert from block major number to name.
 */
const char *
devsw_blk2name(int bmajor)
{
	int i;

	if (bmajor < 0 || bmajor >= nbdevsw)
		return (NULL);

	for (i = 0 ; i < devsw_nconvs ; i++) {
		if (devsw_conv[i].d_name == NULL ||
		    devsw_conv[i].d_bmajor != bmajor)
			continue;
		return (devsw_conv[i].d_name);
	}

	return (NULL);
}

/*
 * Convert from device name to block major number.
 */
int
devsw_name2blk(const char *name, char *devname, size_t namelen)
{
	int len, i;

	if (name == NULL)
		return (-1);

	for (i = 0 ; i < devsw_nconvs ; i++) {
		if (devsw_conv[i].d_name == NULL)
			continue;
		len = strlen(devsw_conv[i].d_name);
		if (len >= devnamelen ||
		    strncmp(devsw_conv[i].d_name, name, len) != 0)
			continue;
		if (devname != NULL) {
			strncpy(devname, name, len);
			devname[len] = '\0';
		}
		return (devsw_conv[i].d_bmajor);
	}

	return (-1);
}

/*
 * Convert from character dev_t to block dev_t.
 */
dev_t
devsw_chr2blk(dev_t cdev)
{
	int cmajor, i;

	if (cdev == NODEV)
		return (NODEV);

	cmajor = major(cdev);
	for (i = 0 ; i < devsw_nconvs ; i++) {
		if (devsw_conv[i].d_name == NULL ||
		    devsw_conv[i].d_cmajor != cmajor)
			continue;
		return (makedev(devsw_conv[i].d_bmajor, minor(cdev)));
	}

	return (NODEV);
};

/*
 * Convert from block dev_t to character dev_t.
 */
dev_t
devsw_blk2chr(dev_t bdev)
{
	int bmajor, i;

	if (bdev == NODEV)
		return (NODEV);

	bmajor = major(bdev);
	for (i = 0 ; i < devsw_nconvs ; i++) {
		if (devsw_conv[i].d_name == NULL ||
		    devsw_conv[i].d_bmajor != bmajor)
			continue;
		return (makedev(devsw_conv[i].d_cmajor, minor(bdev)));
	}

	return (NODEV);
}
