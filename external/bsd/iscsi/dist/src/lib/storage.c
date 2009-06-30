/* $NetBSD: storage.c,v 1.2 2009/06/30 02:44:52 agc Exp $ */

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@netbsd.org)
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
#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
 
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
    
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#include "iscsiprotocol.h"
#include "iscsiutil.h"
#include "target.h"
#include "device.h"

#include "conffile.h"
#include "storage.h"

/* let's use symbolic names for the fields in the config file */
enum {
	EXTENT_NAME_COL = 0,
	EXTENT_DEVICE_COL = 1,
	EXTENT_SACRED_COL = 2,
	EXTENT_LENGTH_COL = 3,

	DEVICE_NAME_COL = 0,
	DEVICE_RAIDLEVEL_COL = 1,
	DEVICE_LENGTH_COL = 2,

	TARGET_NAME_COL = 0,
	TARGET_V1_DEVICE_COL = 1,
	TARGET_V1_NETMASK_COL = 2,
	TARGET_V2_FLAGS_COL = 1,
	TARGET_V2_DEVICE_COL = 2,
	TARGET_V2_NETMASK_COL = 3
};

#define DEFAULT_FLAGS	"ro"

#define TERABYTES(x)	(uint64_t)(1024ULL * 1024ULL * 1024ULL * 1024ULL * (x))
#define GIGABYTES(x)	(uint64_t)(1024ULL * 1024ULL * 1024ULL * (x))
#define MEGABYTES(x)	(uint64_t)(1024ULL * 1024ULL * (x))
#define KILOBYTES(x)	(uint64_t)(1024ULL * (x))

/* find an extent by name */
static disc_extent_t *
find_extent(extv_t *extents, char *s)
{
	size_t	i;

	for (i = 0 ; i < extents->c ; i++) {
		if (strcmp(extents->v[i].extent, s) == 0) {
			return &extents->v[i];
		}
	}
	return NULL;
}

/* allocate space for a new extent */
static int
do_extent(conffile_t *cf, extv_t *extents, ent_t *ep)
{
	disc_extent_t	*extent;
	struct stat	 st;
	char		*cp;

	if (find_extent(extents, ep->sv.v[EXTENT_NAME_COL]) != NULL) {
		(void) fprintf(stderr,
			"%s:%d: Error: attempt to re-define extent `%s'\n",
			conffile_get_name(cf),
			conffile_get_lineno(cf),
			ep->sv.v[EXTENT_NAME_COL]);
		return 0;
	}
	ALLOC(disc_extent_t, extents->v, extents->size, extents->c, 14, 14,
			"do_extent", exit(EXIT_FAILURE));
	extent = &extents->v[extents->c++];
	extent->extent = strdup(ep->sv.v[EXTENT_NAME_COL]);
	extent->dev = strdup(ep->sv.v[EXTENT_DEVICE_COL]);
	extent->sacred = strtoll(ep->sv.v[EXTENT_SACRED_COL],
			NULL, 10);
	if (strcasecmp(ep->sv.v[EXTENT_LENGTH_COL], "size") == 0) {
		if (stat(ep->sv.v[EXTENT_DEVICE_COL], &st) == 0) {
			extent->len = st.st_size;
		}
	} else {
		extent->len = strtoll(ep->sv.v[EXTENT_LENGTH_COL], &cp, 10);
		if (cp != NULL) {
			switch(tolower((unsigned)*cp)) {
			case 't':
				extent->len = TERABYTES(extent->len);
				break;
			case 'g':
				extent->len = GIGABYTES(extent->len);
				break;
			case 'm':
				extent->len = MEGABYTES(extent->len);
				break;
			case 'k':
				extent->len = KILOBYTES(extent->len);
				break;
			}
		}
	}
	return 1;
}

/* find a device by name */
static disc_device_t *
find_device(devv_t *devvp, char *s)
{
	size_t	i;

	for (i = 0 ; i < devvp->c ; i++) {
		if (strcmp(devvp->v[i].dev, s) == 0) {
			return &devvp->v[i];
		}
	}
	return NULL;
}

/* return the size of the sub-device/extent */
static uint64_t
getsize(conffile_t *cf, devv_t *devvp, extv_t *extents, char *s)
{
	disc_extent_t	*xp;
	disc_device_t	*dp;

	if ((xp = find_extent(extents, s)) != NULL) {
		return xp->len;
	}
	if ((dp = find_device(devvp, s)) != NULL) {
		switch (dp->xv[0].type) {
		case DE_EXTENT:
			return dp->xv[0].u.xp->len;
		case DE_DEVICE:
			return dp->xv[0].u.dp->len;
		}
	}
	(void) fprintf(stderr, "%s:%d: Warning: no sub-device/extent `%s'\n",
				conffile_get_name(cf),
				conffile_get_lineno(cf),
				s);
	return 0;
}

/* allocate space for a device */
static int
do_device(conffile_t *cf, devv_t *devvp, extv_t *extents, ent_t *ep)
{
	disc_device_t	*disk;
	char		*device;

	device = ep->sv.v[DEVICE_NAME_COL];
	if ((disk = find_device(devvp, device)) != NULL) {
		(void) fprintf(stderr,
			"%s:%d: Error: attempt to re-define device `%s'\n",
			conffile_get_name(cf),
			conffile_get_lineno(cf),
			device);
		return 0;
	}
	ALLOC(disc_device_t, devvp->v, devvp->size, devvp->c, 14, 14,
				"do_device", exit(EXIT_FAILURE));
	disk = &devvp->v[devvp->c];
	disk->dev = strdup(device);
	disk->raid =
		(strncasecmp(ep->sv.v[DEVICE_RAIDLEVEL_COL], "raid", 4) == 0) ?
		atoi(&ep->sv.v[DEVICE_RAIDLEVEL_COL][4]) : 0;
	disk->size = ep->sv.c - 2;
	disk->len = getsize(cf, devvp, extents, ep->sv.v[DEVICE_LENGTH_COL]);
	NEWARRAY(disc_de_t, disk->xv, ep->sv.c - 2, "do_device",
			exit(EXIT_FAILURE));
	for (disk->c = 0 ; disk->c < disk->size ; disk->c++) {
		disk->xv[disk->c].u.xp =
				find_extent(extents, ep->sv.v[disk->c + 2]);
		if (disk->xv[disk->c].u.xp != NULL) {
			/* a reference to an extent */
			if (disk->xv[disk->c].u.xp->used) {
				(void) fprintf(stderr,
					"%s:%d: "
				"Error: extent `%s' has already been used\n",
					conffile_get_name(cf),
					conffile_get_lineno(cf),
					ep->sv.v[disk->c + 2]);
				return 0;
			}
			if (disk->xv[disk->c].u.xp->len != disk->len &&
							disk->raid != 0) {
				(void) fprintf(stderr,
					"%s:%d: "
					"Error: extent `%s' has size %" PRIu64
					", not %" PRIu64"\n",
					conffile_get_name(cf),
					conffile_get_lineno(cf),
					ep->sv.v[disk->c + 2],
					disk->xv[disk->c].u.xp->len,
					disk->len);
				return 0;
			}
			disk->xv[disk->c].type = DE_EXTENT;
			disk->xv[disk->c].size = disk->xv[disk->c].u.xp->len;
			disk->xv[disk->c].u.xp->used = 1;
		} else if ((disk->xv[disk->c].u.dp =
			find_device(devvp, ep->sv.v[disk->c + 2])) != NULL) {
			/* a reference to a device */
			if (disk->xv[disk->c].u.dp->used) {
				(void) fprintf(stderr,
					"%s:%d: "
				"Error: device `%s' has already been used\n",
					conffile_get_name(cf),
					conffile_get_lineno(cf),
					ep->sv.v[disk->c + 2]);
				return 0;
			}
			disk->xv[disk->c].type = DE_DEVICE;
			disk->xv[disk->c].u.dp->used = 1;
			disk->xv[disk->c].size = disk->xv[disk->c].u.dp->len;
		} else {
			/* not an extent or device */
			(void) fprintf(stderr,
				"%s:%d: "
				"Error: no extent or device found for `%s'\n",
				conffile_get_name(cf),
				conffile_get_lineno(cf),
				ep->sv.v[disk->c + 2]);
			return 0;
		}
	}
	if (disk->raid == 1) {
		/* check we have more than 1 device/extent */
		if (disk->c < 2) {
			(void) fprintf(stderr,
					"%s:%d: Error: device `%s' is RAID1, "
					"but has only %d sub-devices/extents\n",
					conffile_get_name(cf),
					conffile_get_lineno(cf),
					disk->dev, disk->c);
			return 0;
		}
	}
	devvp->c += 1;
	return 1;
}

/* find a target by name */
static disc_target_t *
find_target(targv_t *targs, char *s)
{
	size_t	i;

	for (i = 0 ; i < targs->c ; i++) {
		if (strcmp(targs->v[i].target, s) == 0) {
			return &targs->v[i];
		}
	}
	return NULL;
}

/* allocate space for a new target */
static int
do_target(conffile_t *cf, targv_t *targs, devv_t *devvp, extv_t *extents, ent_t *ep)
{
	disc_extent_t	*xp;
	disc_device_t	*dp;
	const char	*flags;
	char		 tgt[256];
	char		*iqn;
	int		 netmaskcol;
	int		 devcol;

	if ((iqn = strchr(ep->sv.v[TARGET_NAME_COL], '=')) == NULL) {
		(void) strlcpy(tgt, ep->sv.v[TARGET_NAME_COL], sizeof(tgt));
	} else {
		(void) snprintf(tgt, sizeof(tgt), "%.*s",
				(int)(iqn - ep->sv.v[TARGET_NAME_COL]),
				ep->sv.v[TARGET_NAME_COL]);
		iqn += 1;
	}
	if (find_target(targs, tgt) != NULL) {
		(void) fprintf(stderr,
			"%s:%d: Error: attempt to re-define target `%s'\n",
			conffile_get_name(cf),
			conffile_get_lineno(cf),
			tgt);
		return 0;
	}
	ALLOC(disc_target_t, targs->v, targs->size, targs->c, 14, 14,
			"do_target", exit(EXIT_FAILURE));
	if (ep->sv.c == 3) {
		/* 3 columns in entry - old style declaration */
		(void) fprintf(stderr,
				"%s:%d: "
				"Warning: old 3 field \"targets\" entry"
				"assuming read-only target\n",
				conffile_get_name(cf),
				conffile_get_lineno(cf));
		devcol = TARGET_V1_DEVICE_COL;
		netmaskcol = TARGET_V1_NETMASK_COL;
		flags = DEFAULT_FLAGS;
	} else {
		devcol = TARGET_V2_DEVICE_COL;
		flags = ep->sv.v[TARGET_V2_FLAGS_COL];
		netmaskcol = TARGET_V2_NETMASK_COL;
	}
	if (iqn != NULL) {
		targs->v[targs->c].iqn = strdup(iqn);
	}
	if ((dp = find_device(devvp, ep->sv.v[devcol])) != NULL) {
		/* we have a device */
		targs->v[targs->c].de.type = DE_DEVICE;
		targs->v[targs->c].de.u.dp = dp;
		targs->v[targs->c].target = strdup(tgt);
		targs->v[targs->c].mask = strdup(ep->sv.v[netmaskcol]);
		if (strcmp(flags, "readonly") == 0 ||
		    strcmp(flags, "ro") == 0 || strcmp(flags, "r") == 0) {
			targs->v[targs->c].flags |= TARGET_READONLY;
		}
		targs->c += 1;
		return 1;
	}
	if ((xp = find_extent(extents, ep->sv.v[devcol])) != NULL) {
		/* we have an extent */
		targs->v[targs->c].de.type = DE_EXTENT;
		targs->v[targs->c].de.u.xp = xp;
		targs->v[targs->c].target = strdup(tgt);
		targs->v[targs->c].mask = strdup(ep->sv.v[netmaskcol]);
		if (strcmp(flags, "readonly") == 0 ||
		    strcmp(flags, "ro") == 0 || strcmp(flags, "r") == 0) {
			targs->v[targs->c].flags |= TARGET_READONLY;
		}
		targs->c += 1;
		return 1;
	}
	(void) fprintf(stderr,
			"%s:%d: "
			"Error: no device or extent found for `%s'\n",
			conffile_get_name(cf),
			conffile_get_lineno(cf),
			ep->sv.v[devcol]);
	return 0;
}

/* print an extent */
static void
pextent(disc_extent_t *ep, int indent)
{
	int	i;

	for (i = 0 ; i < indent ; i++) {
		(void) fputc('\t', stdout);
	}
	printf("%s:%s:%" PRIu64 ":%" PRIu64 "\n", ep->extent, ep->dev,
			ep->sacred, ep->len);
}

static void pdevice(disc_device_t *, int);

/* print information about an extent or a device */
static void
pu(disc_de_t *dep, int indent)
{
	switch(dep->type) {
	case DE_EXTENT:
		pextent(dep->u.xp, indent);
		break;
	case DE_DEVICE:
		pdevice(dep->u.dp, indent);
		break;
	}
}

/* print information about a device */
static void
pdevice(disc_device_t *dp, int indent)
{
	size_t	j;
	int	i;

	for (i = 0 ; i < indent ; i++) {
		(void) fputc('\t', stdout);
	}
	printf("%s:RAID%d\n", dp->dev, dp->raid);
	for (j = 0 ; j < dp->c ; j++) {
		pu(&dp->xv[j], indent + 1);
	}
}

/* print informnation about a target */
static void
ptarget(disc_target_t *tp, int indent)
{
	int	i;

	for (i = 0 ; i < indent ; i++) {
		(void) fputc('\t', stdout);
	}
	printf("%s:%s:%s\n", tp->target,
		(tp->flags & TARGET_READONLY) ? "ro" : "rw", tp->mask);
	pu(&tp->de, indent + 1);
}

/* print all information */
static void
ptargets(targv_t *targs)
{
	size_t	i;

	for (i = 0 ; i < targs->c ; i++) {
		ptarget(&targs->v[i], 0);
	}
}

/* read a configuration file */
int
read_conf_file(const char *cf, targv_t *targs, devv_t *devs, extv_t *extents)
{
	conffile_t	conf;
	ent_t		e;

	(void) memset(&conf, 0x0, sizeof(conf));
	if (!conffile_open(&conf, cf, "r", " \t", "#")) {
		(void) fprintf(stderr, "Error: can't open `%s'\n", cf);
		return 0;
	}
	printf("Reading configuration from `%s'\n", cf);
	(void) memset(&e, 0x0, sizeof(e));
	while (conffile_getent(&conf, &e)) {
		if (strncmp(e.sv.v[0], "extent", 6) == 0) {
			do_extent(&conf, extents, &e);
		} else if (strncmp(e.sv.v[0], "device", 6) == 0) {
			do_device(&conf, devs, extents, &e);
		} else if (strncmp(e.sv.v[0], "target", 6) == 0 ||
			   strncmp(e.sv.v[0], "lun", 3) == 0) {
			do_target(&conf, targs, devs, extents, &e);
		}
		e.sv.c = 0;
	}
	ptargets(targs);
	(void) conffile_close(&conf);
	return 1;
}

