/*	$NetBSD: subr_devsw.c,v 1.5 2003/02/01 11:12:35 mrg Exp $	*/
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
extern int max_bdevsws, max_cdevsws, max_devsw_convs;

static int bdevsw_attach(const char *, const struct bdevsw *, int *);
static int cdevsw_attach(const char *, const struct cdevsw *, int *);

int
devsw_attach(const char *devname, const struct bdevsw *bdev, int *bmajor,
	     const struct cdevsw *cdev, int *cmajor)
{
	struct devsw_conv *conv;
	char *name;
	int error, i;

	if (devname == NULL || cdev == NULL)
		return (EINVAL);

	for (i = 0 ; i < max_devsw_convs ; i++) {
		conv = &devsw_conv[i];
		if (conv->d_name == NULL || strcmp(devname, conv->d_name) != 0)
			continue;

		if (*bmajor < 0)
			*bmajor = conv->d_bmajor;
		if (*cmajor < 0)
			*cmajor = conv->d_cmajor;

		if (*bmajor != conv->d_bmajor || *cmajor != conv->d_cmajor)
			return (EINVAL);
		if ((*bmajor >= 0 && bdev == NULL) || *cmajor < 0)
			return (EINVAL);

		if ((*bmajor >= 0 && bdevsw[*bmajor] != NULL) ||
		    cdevsw[*cmajor] != NULL)
			return (EEXIST);

		if (bdev != NULL)
			bdevsw[*bmajor] = bdev;
		cdevsw[*cmajor] = cdev;

		return (0);
	}

	error = bdevsw_attach(devname, bdev, bmajor);
	if (error != 0)
		return (error);
	error = cdevsw_attach(devname, cdev, cmajor);
	if (error != 0) {
		devsw_detach(bdev, NULL);
		return (error);
	}

	for (i = 0 ; i < max_devsw_convs ; i++) {
		if (devsw_conv[i].d_name == NULL)
			break;
	}
	if (i == max_devsw_convs) {
		struct devsw_conv *newptr;
		int old, new;

		old = max_devsw_convs;
		new = old + 1;

		newptr = malloc(new * DEVSWCONV_SIZE, M_DEVBUF, M_NOWAIT);
		if (newptr == NULL) {
			devsw_detach(bdev, cdev);
			return (ENOMEM);
		}
		newptr[old].d_name = NULL;
		newptr[old].d_bmajor = -1;
		newptr[old].d_cmajor = -1;
		memcpy(newptr, devsw_conv, old * DEVSWCONV_SIZE);
		if (devsw_conv != devsw_conv0)
			free(devsw_conv, M_DEVBUF);
		devsw_conv = newptr;
		max_devsw_convs = new;
	}

	name = malloc(strlen(devname) + 1, M_DEVBUF, M_NOWAIT);
	if (name == NULL) {
		devsw_detach(bdev, cdev);
		return (ENOMEM);
	}
	strcpy(name, devname);

	devsw_conv[i].d_name = name;
	devsw_conv[i].d_bmajor = *bmajor;
	devsw_conv[i].d_cmajor = *cmajor;

	return (0);
}

static int
bdevsw_attach(const char *devname, const struct bdevsw *devsw, int *devmajor)
{
	int bmajor, i;

	if (devsw == NULL)
		return (0);

	if (*devmajor < 0) {
		for (bmajor = sys_bdevsws ; bmajor < max_bdevsws ; bmajor++) {
			if (bdevsw[bmajor] != NULL)
				continue;
			for (i = 0 ; i < max_devsw_convs ; i++) {
				if (devsw_conv[i].d_bmajor == bmajor)
					break;
			}
			if (i != max_devsw_convs)
				continue;
			break;
		}
		*devmajor = bmajor;
	}
	if (*devmajor >= MAXDEVSW) {
#ifdef DEVSW_DEBUG
		panic("bdevsw_attach: block majors exhausted");
#endif /* DEVSW_DEBUG */
		return (ENOMEM);
	}

	if (*devmajor >= max_bdevsws) {
		const struct bdevsw **newptr;
		int old, new;

		old = max_bdevsws;
		new = *devmajor + 1;

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

	if (bdevsw[*devmajor] != NULL)
		return (EEXIST);

	bdevsw[*devmajor] = devsw;

	return (0);
}

static int
cdevsw_attach(const char *devname, const struct cdevsw *devsw, int *devmajor)
{
	int cmajor, i;

	if (*devmajor < 0) {
		for (cmajor = sys_cdevsws ; cmajor < max_cdevsws ; cmajor++) {
			if (cdevsw[cmajor] != NULL)
				continue;
			for (i = 0 ; i < max_devsw_convs ; i++) {
				if (devsw_conv[i].d_cmajor == cmajor)
					break;
			}
			if (i != max_devsw_convs)
				continue;
			break;
		}
		*devmajor = cmajor;
	}
	if (*devmajor >= MAXDEVSW) {
#ifdef DEVSW_DEBUG
		panic("cdevsw_attach: character majors exhausted");
#endif /* DEVSW_DEBUG */
		return (ENOMEM);
	}

	if (*devmajor >= max_cdevsws) {
		const struct cdevsw **newptr;
		int old, new;

		old = max_cdevsws;
		new = *devmajor + 1;

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
	}

	if (cdevsw[*devmajor] != NULL)
		return (EEXIST);

	cdevsw[*devmajor] = devsw;

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
	int cmajor, i;

	if (bmajor < 0 || bmajor >= max_bdevsws || bdevsw[bmajor] == NULL)
		return (NULL);

	for (i = 0 ; i < max_devsw_convs ; i++) {
		if (devsw_conv[i].d_bmajor != bmajor)
			continue;
		cmajor = devsw_conv[i].d_cmajor;
		if (cmajor < 0 || cmajor >= max_cdevsws ||
		    cdevsw[cmajor] == NULL)
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

	for (i = 0 ; i < max_devsw_convs ; i++) {
		size_t len;

		conv = &devsw_conv[i];
		if (conv->d_name == NULL)
			continue;
		len = strlen(conv->d_name);
		if (strncmp(conv->d_name, name, len) != 0)
			continue;
		if (*(name +len) && !isdigit(*(name + len)))
			continue;
		bmajor = conv->d_bmajor;
		if (bmajor < 0 || bmajor >= max_bdevsws ||
		    bdevsw[bmajor] == NULL)
			break;
		if (devname != NULL) {
#ifdef DEVSW_DEBUG
			if (strlen(conv->d_name) >= devnamelen)
				printf("devsw_name2blk: too short buffer");
#endif /* DEVSW_DEBUG */
			strncpy(devname, conv->d_name, devnamelen);
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
	int bmajor, cmajor, i;

	if (cdevsw_lookup(cdev) == NULL)
		return (NODEV);

	cmajor = major(cdev);

	for (i = 0 ; i < max_devsw_convs ; i++) {
		if (devsw_conv[i].d_cmajor != cmajor)
			continue;
		bmajor = devsw_conv[i].d_bmajor;
		if (bmajor < 0 || bmajor >= max_bdevsws ||
		    bdevsw[bmajor] == NULL)
			return (NODEV);
		return (makedev(bmajor, minor(cdev)));
	}

	return (NODEV);
}

/*
 * Convert from block dev_t to character dev_t.
 */
dev_t
devsw_blk2chr(dev_t bdev)
{
	int bmajor, cmajor, i;

	if (bdevsw_lookup(bdev) == NULL)
		return (NODEV);

	bmajor = major(bdev);

	for (i = 0 ; i < max_devsw_convs ; i++) {
		if (devsw_conv[i].d_bmajor != bmajor)
			continue;
		cmajor = devsw_conv[i].d_cmajor;
		if (cmajor < 0 || cmajor >= max_cdevsws ||
		    cdevsw[cmajor] == NULL)
			return (NODEV);
		return (makedev(cmajor, minor(bdev)));
	}

	return (NODEV);
}
