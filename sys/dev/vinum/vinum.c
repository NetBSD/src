/*-
 * Copyright (c) 1997, 1998
 *	Nan Yang Computer Services Limited.  All rights reserved.
 *
 *  Written by Greg Lehey
 *
 *  This software is distributed under the so-called ``Berkeley
 *  License'':
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
 *	This product includes software developed by Nan Yang Computer
 *      Services Limited.
 * 4. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even if
 * advised of the possibility of such damage.
 *
 * $Id: vinum.c,v 1.1.1.1.4.4 2004/09/21 13:34:15 skrll Exp $
 * $FreeBSD$
 */

#define STATIC static					    /* nothing while we're testing XXX */

#include <dev/vinum/vinumhdr.h>
#ifdef VINUMDEBUG
#include <sys/reboot.h>
int debug = 0;
extern int total_malloced;
extern int malloccount;
extern struct mc malloced[];
#endif
#include <dev/vinum/request.h>

const struct cdevsw vinum_cdevsw =
{
    vinumopen, vinumclose, vinumread, vinumwrite, vinumioctl,
    nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

const struct bdevsw vinum_bdevsw =
{
    vinumopen, vinumclose, vinumstrategy, vinumioctl,
    nodump, vinumsize, D_DISK
};

/* Called by main() during pseudo-device attachment. */
void vinumattach(void *);

struct _vinum_conf vinum_conf;				    /* configuration information */

/*
 * Called by main() during pseudo-device attachment.  All we need
 * to do is allocate enough space for devices to be configured later, and
 * add devsw entries.
 */
void
vinumattach(void *dummy)
{
/* modload should prevent multiple loads, so this is worth a panic */
    if ((vinum_conf.flags & VF_LOADED) != 0)
	panic("vinum: already loaded");

    log(LOG_INFO, "vinum: loaded\n");
#ifdef VINUMDEBUG
    vinum_conf.flags |= VF_LOADED | VF_HASDEBUG;	    /* we're loaded now, and we support debug */
#else
    vinum_conf.flags |= VF_LOADED;			    /* we're loaded now */
#endif

    daemonq = NULL;					    /* initialize daemon's work queue */
    dqend = NULL;

    vinum_conf.version = VINUMVERSION;			    /* note what version we are */

    /* allocate space: drives... */
    DRIVE = (struct drive *) Malloc(sizeof(struct drive) * INITIAL_DRIVES);
    CHECKALLOC(DRIVE, "vinum: no memory\n");
    bzero(DRIVE, sizeof(struct drive) * INITIAL_DRIVES);
    vinum_conf.drives_allocated = INITIAL_DRIVES;	    /* number of drive slots allocated */
    vinum_conf.drives_used = 0;				    /* and number in use */

    /* volumes, ... */
    VOL = (struct volume *) Malloc(sizeof(struct volume) * INITIAL_VOLUMES);
    CHECKALLOC(VOL, "vinum: no memory\n");
    bzero(VOL, sizeof(struct volume) * INITIAL_VOLUMES);
    vinum_conf.volumes_allocated = INITIAL_VOLUMES;	    /* number of volume slots allocated */
    vinum_conf.volumes_used = 0;			    /* and number in use */

    /* plexes, ... */
    PLEX = (struct plex *) Malloc(sizeof(struct plex) * INITIAL_PLEXES);
    CHECKALLOC(PLEX, "vinum: no memory\n");
    bzero(PLEX, sizeof(struct plex) * INITIAL_PLEXES);
    vinum_conf.plexes_allocated = INITIAL_PLEXES;	    /* number of plex slots allocated */
    vinum_conf.plexes_used = 0;				    /* and number in use */

    /* and subdisks */
    SD = (struct sd *) Malloc(sizeof(struct sd) * INITIAL_SUBDISKS);
    CHECKALLOC(SD, "vinum: no memory\n");
    bzero(SD, sizeof(struct sd) * INITIAL_SUBDISKS);
    vinum_conf.subdisks_allocated = INITIAL_SUBDISKS;	    /* number of sd slots allocated */
    vinum_conf.subdisks_used = 0;			    /* and number in use */
}

/*
 * Check if we have anything open.  If confopen is != 0,
 * that goes for the super device as well, otherwise
 * only for volumes.
 *
 * Return 0 if not inactive, 1 if inactive.
 */
int
vinum_inactive(int confopen)
{
    int i;
    int can_do = 1;					    /* assume we can do it */

    if (confopen && (vinum_conf.flags & VF_OPEN))	    /* open by vinum(8)? */
	return 0;					    /* can't do it while we're open */
    lock_config();
    for (i = 0; i < vinum_conf.volumes_allocated; i++) {
	if ((VOL[i].state > volume_down)
	    && (VOL[i].flags & VF_OPEN)) {		    /* volume is open */
	    can_do = 0;
	    break;
	}
    }
    unlock_config();
    return can_do;
}

/*
 * Free all structures.
 * If cleardrive is 0, save the configuration; otherwise
 * remove the configuration from the drive.
 *
 * Before coming here, ensure that no volumes are open.
 */
void
free_vinum(int cleardrive)
{
    int i;
    int drives_allocated = vinum_conf.drives_allocated;

    if (DRIVE != NULL) {
	if (cleardrive) {				    /* remove the vinum config */
	    for (i = 0; i < drives_allocated; i++)
		remove_drive(i);			    /* remove the drive */
	} else {					    /* keep the config */
	    for (i = 0; i < drives_allocated; i++)
		free_drive(&DRIVE[i]);			    /* close files and things */
	}
	Free(DRIVE);
    }
    while ((vinum_conf.flags & (VF_STOPPING | VF_DAEMONOPEN))
	== (VF_STOPPING | VF_DAEMONOPEN)) {		    /* at least one daemon open, we're stopping */
	queue_daemon_request(daemonrq_return, (union daemoninfo) 0); /* stop the daemon */
	tsleep(&vinumclose, PUSER, "vstop", 1);		    /* and wait for it */
    }
    if (SD != NULL) {
	for (i = 0; i < vinum_conf.subdisks_allocated; i++) {
	    struct sd *sd = &SD[i];

	    if (sd->state != sd_unallocated)
		free_sd(i);
	}
	Free(SD);
    }
    if (PLEX != NULL) {
	for (i = 0; i < vinum_conf.plexes_allocated; i++) {
	    struct plex *plex = &PLEX[i];

	    if (plex->state != plex_unallocated)	    /* we have real data there */
		free_plex(i);
	}
	Free(PLEX);
    }
    if (VOL != NULL) {
	for (i = 0; i < vinum_conf.volumes_allocated; i++) {
	    struct volume *volume = &VOL[i];

	    if (volume->state != volume_unallocated)
		free_volume(i);
	}
	Free(VOL);
    }
    bzero(&vinum_conf, sizeof(vinum_conf));
    vinum_conf.version = VINUMVERSION;			    /* reinstate version number */
}


/* ARGSUSED */
/* Open a vinum object */
int
vinumopen(dev_t dev,
    int flags,
    int fmt,
    struct proc *p)
{
    int error;
    unsigned int index;
    struct volume *vol;
    struct plex *plex;
    struct sd *sd;
    int devminor;					    /* minor number */

    devminor = minor(dev);
    error = 0;
    /* First, decide what we're looking at */
    switch (DEVTYPE(dev)) {
    case VINUM_VOLUME_TYPE:
	/*
	 * The super device and daemon device are the last two
	 * volume numbers, so check for them first.
	 */
	if ((devminor == VINUM_DAEMON_VOL)		    /* daemon device */
	||(devminor == VINUM_SUPERDEV_VOL)) {		    /* or normal super device */
	    error = suser(p->p_ucred, &p->p_acflag);	    /* are we root? */

	    if (error == 0) {				    /* yes, can do */
		if (devminor == VINUM_DAEMON_VOL)	    /* daemon device */
		    vinum_conf.flags |= VF_DAEMONOPEN;	    /* we're open */
		else if (devminor == VINUM_SUPERDEV_VOL)
		    vinum_conf.flags |= VF_OPEN;	    /* we're open */
	    }
	    return error;
	}
	/* Must be a real volume.  Check. */
	index = Volno(dev);
	if (index >= vinum_conf.volumes_allocated)
	    return ENXIO;				    /* no such device */
	vol = &VOL[index];

	switch (vol->state) {
	case volume_unallocated:
	case volume_uninit:
	    return ENXIO;

	case volume_up:
	    vol->flags |= VF_OPEN;			    /* note we're open */
	    return 0;

	case volume_down:
	    return EIO;

	default:
	    return EINVAL;
	}

    case VINUM_PLEX_TYPE:
	index = Plexno(dev);				    /* get plex index in vinum_conf */
	if (index >= vinum_conf.plexes_allocated)
	    return ENXIO;				    /* no such device */
	plex = &PLEX[index];

	switch (plex->state) {
	case plex_unallocated:
	    return ENXIO;

	case plex_referenced:
	    return EINVAL;

	default:
	    plex->flags |= VF_OPEN;			    /* note we're open */
	    return 0;
	}

    case VINUM_SD_TYPE:
    case VINUM_SD2_TYPE:
	index = Sdno(dev);				    /* get the subdisk number */
	if (index >= vinum_conf.subdisks_allocated)	    /* not a valid SD entry */
	    return ENXIO;				    /* no such device */
	sd = &SD[index];

	/*
	 * Opening a subdisk is always a special operation, so
	 * we ignore the state as long as it represents a real
	 * subdisk.
	 */
	switch (sd->state) {
	case sd_unallocated:
	    return ENXIO;

	case sd_uninit:
	case sd_referenced:
	    return EINVAL;

	default:
	    sd->flags |= VF_OPEN;			    /* note we're open */
	    return 0;
	}
    }
    return 0;						    /* to keep the compiler happy */
}

/* ARGSUSED */
int
vinumclose(dev_t dev,
    int flags,
    int fmt,
    struct proc *p)
{
    unsigned int index;
    struct volume *vol;
    int devminor;

    devminor = minor(dev);
    /* First, decide what we're looking at */
    switch (DEVTYPE(dev)) {
    case VINUM_VOLUME_TYPE:
	/*
	 * The super device and daemon device are the last two
	 * volume numbers, so check for them first.
	 */
	if ((devminor == VINUM_DAEMON_VOL)		    /* daemon device */
	||(devminor == VINUM_SUPERDEV_VOL)) {		    /* or normal super device */
	    /*
	     * don't worry about whether we're root:
	     * nobody else would get this far.
	     */
	    if (devminor == VINUM_SUPERDEV_VOL)		    /* normal superdev */
		vinum_conf.flags &= ~VF_OPEN;		    /* no longer open */
	    else {					    /* the daemon device */
		vinum_conf.flags &= ~VF_DAEMONOPEN;	    /* no longer open */
		if (vinum_conf.flags & VF_STOPPING)	    /* we're trying to stop, */
		    wakeup(&vinumclose);		    /* we can continue now */
	    }
	    return 0;
	}
	/* Real volume */
	index = Volno(dev);
	if (index >= vinum_conf.volumes_allocated)
	    return ENXIO;				    /* no such device */
	vol = &VOL[index];

	switch (vol->state) {
	case volume_unallocated:
	case volume_uninit:
	    return ENXIO;

	case volume_up:
	    vol->flags &= ~VF_OPEN;			    /* reset our flags */
	    return 0;

	case volume_down:
	    return EIO;

	default:
	    return EINVAL;
	}

    case VINUM_PLEX_TYPE:
	if (Volno(dev) >= vinum_conf.volumes_allocated)
	    return ENXIO;
	/* FALLTHROUGH */

    case VINUM_SD_TYPE:
	if ((Volno(dev) >= vinum_conf.volumes_allocated) || /* no such volume */
	    (Plexno(dev) >= vinum_conf.plexes_allocated))   /* or no such plex */
	    return ENXIO;				    /* no such device */
	/* FALLTHROUGH */

    default:
	return ENODEV;					    /* don't know what to do with these */
    }
}


/* size routine */
int
vinumsize(dev_t dev)
{
    struct volume *vol;
    int size;

    vol = &VOL[Volno(dev)];

    if (vol->state == volume_up)
	size = vol->size;
    else
	return 0;					    /* err on the size of conservatism */

    return size;
}

int
vinumdump(dev_t dev, daddr_t da, caddr_t ca, size_t size)
{
    return ENXIO;					    /* Implement this! XXX. */
}

/* Local Variables: */
/* fill-column: 60 */
/* End: */
