/*	$NetBSD: devicename.c,v 1.7 2014/04/08 21:51:06 martin Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/ia64/libski/devicename.c,v 1.2 2003/09/08 09:11:32 obrien Exp $"); */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>
#include <sys/disklabel.h>

#include <bootstrap.h>
#include "libski.h"

static int	ski_parsedev(struct ski_devdesc **dev, const char *devspec, const char **path);

/* 
 * Point (dev) at an allocated device specifier for the device matching the
 * path in (devspec). If it contains an explicit device specification,
 * use that.  If not, use the default device.
 */
int
ski_getdev(void **vdev, const char *devspec, const char **path)
{
	struct ski_devdesc **dev = (struct ski_devdesc **)vdev;
	int		rv;
    
	/*
	 * If it looks like this is just a path and no
	 * device, go with the current device.
	 */
	if ((devspec == NULL) || 
	    (devspec[0] == '/') || 
	    (strchr(devspec, ':') == NULL)) {

		if (((rv = ski_parsedev(dev, getenv("currdev"), NULL)) == 0) &&
		    (path != NULL))
			*path = devspec;
		return(rv);
	}
    
	/*
	 * Try to parse the device name off the beginning of the devspec
	 */
	return(ski_parsedev(dev, devspec, path));
}

/*
 * Point (dev) at an allocated device specifier matching the string version
 * at the beginning of (devspec).  Return a pointer to the remaining
 * text in (path).
 *
 * In all cases, the beginning of (devspec) is compared to the names
 * of known devices in the device switch, and then any following text
 * is parsed according to the rules applied to the device type.
 *
 * For disk-type devices, the syntax is:
 *
 * disk<unit>[s<slice>][<partition>]:
 * 
 */
static int
ski_parsedev(struct ski_devdesc **dev, const char *devspec, const char **path)
{
	struct ski_devdesc *idev;
	struct devsw	*dv;
	int dv_type;
	int		i, unit, slice, partition, err;
	char		*cp;
	const char	*np;

	/* minimum length check */
	if (strlen(devspec) < 2)
		return(EINVAL);

	/* look for a device that matches */
	for (i = 0, dv = NULL; i < ndevs; i++) {
		if (!strncmp(devspec, devsw[i].dv_name, strlen(devsw[i].dv_name))) {
			dv = &devsw[i];
			break;
		}
	}

	if (dv == NULL)
		return(ENOENT);
	idev = alloc(sizeof(struct ski_devdesc));
	err = 0;
	np = (devspec + strlen(dv->dv_name));
        
	/* XXX: Fixme */
	dv_type = DEVT_DISK;

	switch(dv_type) {
	case DEVT_NONE:			/* XXX what to do here?  Do we care? */
		break;

	case DEVT_DISK:			/* XXX: Need to fix DEVT_DISK for NetBSD disk conventions. */
		unit = -1;
		slice = -1;
		partition = -1;
		if (*np && (*np != ':')) {
			unit = strtol(np, &cp, 10);	/* next comes the unit number */
			if (cp == np) {
				err = EUNIT;
				goto fail;
			}
			if (*cp == 's') {		/* got a slice number */
				np = cp + 1;
				slice = strtol(np, &cp, 10);
				if (cp == np) {
					err = EPART;
					goto fail;
				}
			}
			if (*cp && (*cp != ':')) {
				partition = *cp - 'a';		/* get a partition number */
				if ((partition < 0) || (partition >= MAXPARTITIONS)) {
					err = EPART;
					goto fail;
				}
				cp++;
			}
		}
		if (*cp && (*cp != ':')) {
			err = EINVAL;
			goto fail;
		}

		idev->d_kind.skidisk.unit = unit;
		idev->d_kind.skidisk.slice = slice;
		idev->d_kind.skidisk.partition = partition;

		if (path != NULL)
			*path = (*cp == 0) ? cp : cp + 1;
		break;
	
	case DEVT_NET:
		unit = 0;
	
		if (*np && (*np != ':')) {
			unit = strtol(np, &cp, 0);	/* get unit number if present */
			if (cp == np) {
				err = EUNIT;
				goto fail;
			}
		}
		if (*cp && (*cp != ':')) {
			err = EINVAL;
			goto fail;
		}
	
		idev->d_kind.netif.unit = unit;
		if (path != NULL)
			*path = (*cp == 0) ? cp : cp + 1;
		break;

	default:
		err = EINVAL;
		goto fail;
	}
	idev->d_dev = dv;
	idev->d_type = dv_type;
	if (dev == NULL) {
		free(idev);
	} else {
		*dev = idev;
	}
	return(0);

 fail:
	free(idev);
	return(err);
}


char *
ski_fmtdev(void *vdev)
{
	struct ski_devdesc *dev = (struct ski_devdesc *)vdev;
	static char	buf[128];	/* XXX device length constant? */
	size_t len, buflen = sizeof(buf);
    
	switch(dev->d_type) {
	case DEVT_NONE:
		strlcpy(buf, "(no device)", buflen);
		break;

	case DEVT_DISK:
		len = snprintf(buf, buflen, "%s%d", dev->d_dev->dv_name, dev->d_kind.skidisk.unit);
		if (len > buflen)
			len = buflen;
		if (dev->d_kind.skidisk.slice > 0) {
			len += snprintf(buf + len, buflen - len, "s%d", dev->d_kind.skidisk.slice);
			if (len > buflen)
				len = buflen;
		}
		if (dev->d_kind.skidisk.partition >= 0) {
			len += snprintf(buf + len, buflen - len, "%c", dev->d_kind.skidisk.partition + 'a');
			if (len > buflen)
				len = buflen;
		}
		strlcat(buf, ":", buflen - len);
		break;

	case DEVT_NET:
		snprintf(buf, buflen - len, "%s%d:", dev->d_dev->dv_name, dev->d_kind.netif.unit);
		break;
	}
	return(buf);
}


/*
 * Set currdev to suit the value being supplied in (value)
 */
int
ski_setcurrdev(struct env_var *ev, int flags, void *value)
{
	struct ski_devdesc *ncurr;
	int		rv;
    
	if ((rv = ski_parsedev(&ncurr, value, NULL)) != 0)
		return(rv);
	free(ncurr);
	env_setenv(ev->ev_name, flags | EV_NOHOOK, value, NULL, NULL);
	return(0);
}

