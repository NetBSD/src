/*	$NetBSD: subr_devsw.c,v 1.1.2.5 2002/05/22 10:57:12 gehenna Exp $	*/
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
extern const int sys_bdevsws, sys_cdevsws;
extern int max_bdevsws, max_cdevsws;

int
devsw_attach(const char *devname, const struct bdevsw *bdev, int *bmajor,
	     const struct cdevsw *cdev, int *cmajor)
{
	int i;

	if (devname == NULL || cdev == NULL)
		return (EINVAL);

	if (bdevsw_lookup_major(bdev) != -1 ||
	    cdevsw_lookup_major(cdev) != -1)
		return (EEXIST);

	if (*cmajor < 0) {
		for (i = 0 ; i < max_cdevsws ; i++) {
			if (devsw_conv[i].d_name == NULL ||
			    strcmp(devname, devsw_conv[i].d_name) != 0)
				continue;
			*cmajor = i;
			if (devsw_conv[i].d_bmajor != -1) {
				if (bdev == NULL)
					return (EINVAL);
				if (*bmajor != -1 &&
				    *bmajor != devsw_conv[i].d_bmajor)
					return (EINVAL);
				*bmajor = devsw_conv[i].d_bmajor;
			}
			break;
		}
	}

	if (bdev != NULL) {
		if (*bmajor < 0) {
			for (i = sys_bdevsws ; i < max_bdevsws ; i++) {
				int j;

				if (bdevsw[i] != NULL)
					continue;
				for (j = sys_cdevsws ; j < max_cdevsws ; j++) {
					if (i == devsw_conv[i].d_bmajor)
						break;
				}
				if (j != max_cdevsws)
					continue;
				*bmajor = i;
				break;
			}
			if (*bmajor < 0)
				*bmajor = max_bdevsws;
			if (*bmajor >= MAXDEVSW)
				return (EINVAL);
		}

		if (*bmajor >= max_bdevsws) {
			int old = max_bdevsws, new = *bmajor + 1;
			const struct bdevsw **newptr;

			newptr = malloc(new * BDEVSW_SIZE, M_DEVBUF, M_NOWAIT);
			if (newptr == NULL)
				return (ENOMEM);
			memset(newptr + old, 0, (new - old) * BDEVSW_SIZE);
			if (old != 0) {
				memcpy(newptr, bdevsw, old * BDEVSW_SIZE);
				if (bdevsw != bdevsw0)
					free(bdevsw, M_DEVBUF);
			}
			bdevsw = newptr;
			max_bdevsws = new;
		}

		if (bdevsw[*bmajor] != NULL)
			return (EEXIST);
	}

	if (*cmajor < 0) {
		for (i = sys_cdevsws ; i < max_cdevsws ; i++) {
			if (cdevsw[i] != NULL || devsw_conv[i].d_name != NULL)
				continue;
			*cmajor = i;
			break;
		}
		if (*cmajor < 0)
			*cmajor = max_cdevsws;
		if (*cmajor >= MAXDEVSW)
			return (EINVAL);
	}

	if (*cmajor >= max_cdevsws) {
		int old = max_cdevsws, new = *cmajor + 1;
		const struct cdevsw **newptr;
		struct devsw_conv *newconv;

		newptr = malloc(new * CDEVSW_SIZE, M_DEVBUF, M_NOWAIT);
		if (newptr == NULL)
			return (ENOMEM);
		memset(newptr + old, 0, (new - old) * CDEVSW_SIZE);
		if (old != 0) {
			memcpy(newptr, cdevsw, old * CDEVSW_SIZE);
			if (cdevsw != cdevsw0)
				free(cdevsw, M_DEVBUF);
		}
		cdevsw = newptr;
		max_cdevsws = new;

		newconv = malloc(new * DEVSWCONV_SIZE, M_DEVBUF, M_NOWAIT);
		if (newconv == NULL)
			return (ENOMEM);
		for (i = old ; i < new ; i++) {
			newconv[i].d_name = NULL;
			newconv[i].d_bmajor = -1;
		}
		if (old != 0) {
			memcpy(newconv, devsw_conv, old * DEVSWCONV_SIZE);
			if (devsw_conv != devsw_conv0)
				free(devsw_conv, M_DEVBUF);
		}
		devsw_conv = newconv;
	}

	if (cdevsw[*cmajor] != NULL)
		return (EEXIST);

	if (devsw_conv[*cmajor].d_name == NULL) {
		char *name;
		name = malloc(strlen(devname) + 1, M_DEVBUF, M_NOWAIT);
		if (name == NULL)
			return (ENOMEM);
		strcpy(name, devname);
		devsw_conv[*cmajor].d_name = name;
	}

	if (bdev != NULL) {
		bdevsw[*bmajor] = bdev;
		devsw_conv[*cmajor].d_bmajor = *bmajor;
	}
	cdevsw[*cmajor] = cdev;

	return (0);
}

void
devsw_detach(const struct bdevsw *bdev, const struct cdevsw *cdev)
{
	int i;

	if (bdev != NULL) {
		for (i = 0 ; i < max_bdevsws ; i++) {
			if (bdevsw[i] != bdev)
				continue;
			bdevsw[i] = NULL;
			break;
		}
	}
	if (cdev != NULL) {
		for (i = 0 ; i < max_cdevsws ; i++) {
			if (cdevsw[i] != cdev)
				continue;
			cdevsw[i] = NULL;
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
	if (bmajor < 0 || bmajor >= max_bdevsws)
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
	if (cmajor < 0 || cmajor >= max_cdevsws)
		return (NULL);

	return (cdevsw[cmajor]);
}

int
bdevsw_lookup_major(const struct bdevsw *bdev)
{
	int bmajor;

	for (bmajor = 0 ; bmajor < max_bdevsws ; bmajor++) {
		if (bdevsw[bmajor] == bdev)
			return (bmajor);
	}

	return (-1);
}

int
cdevsw_lookup_major(const struct cdevsw *cdev)
{
	int cmajor;

	for (cmajor = 0 ; cmajor < max_cdevsws ; cmajor++) {
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

	if (bmajor < 0 || bmajor >= max_bdevsws)
		return (NULL);
	if (bdevsw[bmajor] == NULL)
		return (NULL);

	for (i = 0 ; i < max_cdevsws ; i++) {
		if (devsw_conv[i].d_bmajor != bmajor)
			continue;
		if (cdevsw[i] == NULL)
			return (NULL);
		return (devsw_conv[i].d_name);
	}

	return (NULL);
}

/*
 * Convert from device name to block major number.
 */
int
devsw_name2blk(const char *name, char *devname, size_t devnamelen)
{
	struct devsw_conv *conv;
	int bmajor, i;

	if (name == NULL)
		return (-1);

	for (i = 0 ; i < max_cdevsws ; i++) {
		conv = &devsw_conv[i];
		if (conv->d_name == NULL)
			continue;
		if (strncmp(conv->d_name, name, strlen(conv->d_name)) != 0)
			continue;
		bmajor = conv->d_bmajor;
		if (bmajor < 0 || bmajor >= max_bdevsws ||
		    bdevsw[bmajor] == NULL)
			return (-1);
		if (devname != NULL) {
#ifdef DEVSW_DEBUG
			if (strlen(conv->d_name) >= devnamelen)
				printf("devsw_debug: too short buffer");
#endif /* DEVSW_DEBUG */
			strncpy(devname, name, devnamelen);
			devname[devnamelen - 1] = '\0';
		}
		return (bmajor);
	}

	return (-1);
}

/*
 * Convert from character dev_t to block dev_t.
 */
dev_t
devsw_chr2blk(dev_t cdev)
{
	int bmajor;

	if (cdevsw_lookup(cdev) == NULL)
		return (NODEV);

	bmajor = devsw_conv[major(cdev)].d_bmajor;
	if (bmajor < 0 || bmajor >= max_bdevsws || bdevsw[bmajor] == NULL)
		return (NODEV);

	return (makedev(bmajor, minor(cdev)));
};

/*
 * Convert from block dev_t to character dev_t.
 */
dev_t
devsw_blk2chr(dev_t bdev)
{
	int bmajor, i;

	if (bdevsw_lookup(bdev) == NULL)
		return (NODEV);

	bmajor = major(bdev);

	for (i = 0 ; i < max_cdevsws ; i++) {
		if (devsw_conv[i].d_bmajor != bmajor)
			continue;
		if (cdevsw[i] == NULL)
			return (NODEV);
		return (makedev(i, minor(bdev)));
	}

	return (NODEV);
}
