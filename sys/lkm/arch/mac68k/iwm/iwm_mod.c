/* $Id: iwm_mod.c,v 1.1 1999/03/26 22:25:40 scottr Exp $ */

/*
 * Copyright (c) 1997, 1998 Hauke Fath.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

/*
 * Sony (floppy disk) driver for Macintosh m68k, module entry.
 * This is derived from Terry Lambert's LKM examples.
 */
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

#include "iwm_mod.h"


/*
 * From kern/kern_lkm.c
 * XXX If it's used outside, it should appear in the header!
 */
int lkmexists __P((struct lkm_table *));
int lkmdispatch __P((struct lkm_table *, int));

/* The module entry */
int IWM_lkmentry __P((struct lkm_table *lkmtp, int cmd, int ver));

/* local */
static int load_module(struct lkm_table * lkmtp, int cmd);
static int unload_module(struct lkm_table * lkmtp, int cmd);


/*
 * Provide standard device driver entry points
 * (Macros are in <sys/conf.h>, the variables {b,c}devsw[] live in
 * arch/mac68k/mac68k/conf.c).
 *
 * XXX Macros in <sys/conf.h> don't compile cleanly with -Werror.
 */
static struct bdevsw newBDevEntry = {
	fdopen,
	fdclose,
	fdstrategy,
	fdioctl,
	(dev_type_dump((*))) fddump,
	fdsize,
	D_DISK
};

static struct cdevsw newCDevEntry = {
	fdopen,
	fdclose,
	fdread,
	fdwrite,
	fdioctl,
	(dev_type_stop((*))) enodev,
	0,
	seltrue,
	(dev_type_mmap((*))) enodev,
	D_DISK
};


/*
 * Store away the old device driver switch entries for cleanup when we unload.
 */
static struct bdevsw oldBDevEntry;
static struct cdevsw oldCDevEntry;

static struct lkm_misc _module = {
	LM_MISC,
	LKM_VERSION,
	"IWM_lkmentry"
};


/*
 * These functions are called each time the module is loaded or unloaded.
 *
 * Although the LKM interface provides an instance for device drivers,
 * we have to roll our own for a disk driver: We need to patch both,
 * block _and_ character driver entries.
 */


/*
 * load_module
 *
 * Check if already loaded and patch device driver switch table entries.
 */
static int
load_module(struct lkm_table *lkmtp,
	   int cmd)
{
	int     i;
	int     err;
	struct lkm_misc *args;

	i = 0;
	args = lkmtp->private.lkm_misc;
#ifdef DEBUG
	printf("iwm: Calling iwmModuleHandler()...\n");
#endif
	/* Don't load twice! (lkmexists() is exported by kern_lkm.c) */
	err = (0 == lkmexists(lkmtp)) ? 0 : EEXIST;

	if (!err) {
		/*
		 * We would like to see the block device in slot #2
		 * and the char device in slot #18.
		 * For now, we enforce this.
		 */

		/* save old -- we must provide our own data area */
		bcopy(&bdevsw[2], &oldBDevEntry, sizeof(struct bdevsw));
		bcopy(&cdevsw[18], &oldCDevEntry, sizeof(struct cdevsw));

		/* replace with new */
		bcopy(&newBDevEntry, &bdevsw[2], sizeof(struct bdevsw));
		bcopy(&newCDevEntry, &cdevsw[18], sizeof(struct cdevsw));
		/* 
		 * If we wanted to allocate device nodes in /dev, 
		 * we could export the numbers here. 
		 * For the floppy devices, we assume they
		 * have already been allocated by /dev/MAKEDEV. 
		 */
		args->lkm_offset = i;
		err = fd_mod_init();
	}
	if (!err) {
		printf("IWM floppy disk driver kernel module.\n");
		printf("Copyright (c) 1996-1998 Hauke Fath. ");
		printf("All rights reserved.\n");
	}
	return (err);
}


/*
 * unload_module
 *
 * Free any occupied resources and restore patched device driver
 * switch entries.
 */
static int
unload_module(struct lkm_table * lkmtp,
	     int cmd)
{
	int     i;
	int     err;
	struct lkm_misc *args;

	i = 0;
	err = 0;
	args = lkmtp->private.lkm_misc;

#ifdef DEBUG
	printf("iwm: Calling unloadModule()...\n");
#endif
	i = args->lkm_offset;	/* current slot, unused	 */
	fd_mod_free();

	/* replace current slot contents with old contents */
	bcopy(&oldBDevEntry, &bdevsw[2], sizeof(struct bdevsw));
	bcopy(&oldCDevEntry, &cdevsw[18], sizeof(struct cdevsw));

	return (err);
}


/*
 * IWM_lkmentry
 *
 * External entry point; should generally match name of .o file.
 *
 * XXX The DISPATCH macro from <sys/lkm.h> that was originally used here
 * 	does not compile noiselessly with -Werror.
 */
int
IWM_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;		  
	int cmd;
	int ver;
{
	int err;

	err = (ver == LKM_VERSION) ? 0 : EINVAL;	/* version mismatch */

	if (!err) {
		switch (cmd) {
		case LKM_E_LOAD:
			lkmtp->private.lkm_any = (struct lkm_any *) & _module;
			err = load_module(lkmtp, cmd);
			break;

		case LKM_E_UNLOAD:
			err = unload_module(lkmtp, cmd);
			break;

		case LKM_E_STAT:
			err = lkm_nofunc(lkmtp, cmd);
			break;
		}
	}
	if (!err)
		err = lkmdispatch(lkmtp, cmd);

	return (err);
}
