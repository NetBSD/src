/*	$NetBSD: if_tap_lkm.c,v 1.4 2006/02/01 05:51:58 cube Exp $	*/

/*
 *  Copyright (c) 2003, 2004, 2005 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *   by Quentin Garnier.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * tap is a NetBSD Loadable Kernel Module that demonstrates the use of
 * several kernel mechanisms, mostly in the networking subsytem.
 *
 * 1. it is example LKM, with the standard LKM management routines and
 * 2. example Ethernet driver.
 * 3. example of use of autoconf stuff inside a LKM.
 * 4. example clonable network interface.
 * 5. example sysctl interface use from a LKM.
 * 6. example LKM character device, with read, write, ioctl, poll
 *    and kqueue available.
 * 7. example cloning device, using the MOVEFD semantics.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_tap_lkm.c,v 1.4 2006/02/01 05:51:58 cube Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ksyms.h>
#include <sys/lkm.h>
#include <sys/sysctl.h>

#include <net/if.h>

/* autoconf(9) structures */

CFDRIVER_DECL(tap, DV_DULL, NULL);

/* LKM management routines */

int		tap_lkmentry(struct lkm_table *, int, int);
static int	tap_lkmload(struct lkm_table *, int);
static int	tap_lkmunload(struct lkm_table *, int);

void		tapattach(int);
int		tapdetach(void);
int		tap_clone_destroyer(struct device *);
SYSCTL_SETUP_PROTO(sysctl_tap_setup);

extern struct cfattach tap_ca;
extern const struct cdevsw tap_cdevsw;
extern struct if_clone tap_cloners;
static struct sysctllog *tap_log;

/*
 * The type of the module is actually userland-oriented.  For a
 * traditional Ethernet driver, MOD_MISC would be enough since
 * the userland manipulates interfaces through operations on
 * sockets.
 *
 * Here MOD_DEV is chosen because a direct access interface is
 * exposed, and the easiest way to achieve this is through a
 * regular device node.
 */

MOD_DEV("tap", "tap", NULL, -1, &tap_cdevsw, -1);

/* We don't have anything to do on 'modstat' */
int
tap_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver,
	    tap_lkmload, tap_lkmunload, lkm_nofunc);
}

/*
 * autoconf(9) is a rather complicated piece of work, but in the end
 * it is rather flexible, so you can easily add a device somewhere in
 * the tree, and make almost anything attach to something known.
 *
 * Here the idea is taken from Jason R. Thorpe's ataraid(4) pseudo-
 * device.  Instead of needing a declaration in the kernel
 * configuration, we teach autoconf(9) the availability of the
 * pseudo-device at run time.
 *
 * Once our autoconf(9) structures are committed to the kernel's
 * arrays, we can attach a device.  It is done through config_attach
 * for a real device, but for a pseudo-device it is a bit different
 * and one has to use config_pseudo_attach.
 *
 * And since we want the user to be responsible for creating device,
 * we use the interface cloning mechanism, and advertise our interface
 * to the kernel.
 */
static int
tap_lkmload(struct lkm_table *lkmtp, int cmd)
{
	int error = 0;

	error = config_cfdriver_attach(&tap_cd);
	if (error) {
		aprint_error("%s: unable to register cfdriver\n",
		    tap_cd.cd_name);
		goto out;
	}

	/* XXX: no way to detect an error for config_cfattach_attach() */
	tapattach(1);

	sysctl_tap_setup(&tap_log);
out:
	return error;
}

/*
 * Cleaning up is the most critical part of a LKM, since a module is not
 * actually made to be loadable, but rather "unloadable".  If it is only
 * to be loaded, you'd better link it to the kernel in the first place.
 *
 * The interface cloning mechanism is really simple, with only two void
 * returning functions.  It will always do its job. You should note though
 * that if an instance of tap can't be detached, the module won't
 * unload and you won't be able to create interfaces anymore.
 *
 * We have to make sure the devices really exist, because they can be
 * destroyed through ifconfig, hence the test whether cd_devs[i] is NULL
 * or not.
 *
 * The cd_devs array is somehow the downside of the whole autoconf(9)
 * mechanism, since if you only create 'tap150', you'll get an array of
 * 150 elements which 149 of them are NULL.
 */
static int
tap_lkmunload(struct lkm_table *lkmtp, int cmd)
{
	int error, i;

	if_clone_detach(&tap_cloners);

	for (i = 0; i < tap_cd.cd_ndevs; i++)
		if (tap_cd.cd_devs[i] != NULL &&
		    (error = tap_clone_destroyer(tap_cd.cd_devs[i])) != 0)
			return error;

	sysctl_teardown(&tap_log);

	if ((error = config_cfattach_detach(tap_cd.cd_name,
	    &tap_ca)) != 0) {
		aprint_error("%s: unable to deregister cfattach\n",
		    tap_cd.cd_name);
		return error;
	}

	if ((error = config_cfdriver_detach(&tap_cd)) != 0) {
		aprint_error("%s: unable to deregister cfdriver\n",
		    tap_cd.cd_name);
		return error;
	}

	return 0;
}
