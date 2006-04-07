/*	 $NetBSD: mem.c,v 1.1 2006/04/07 14:21:18 cherry Exp $	*/

/*
 * Memory special file
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/conf.h>

const struct cdevsw mem_cdevsw = {
	noopen, noclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

