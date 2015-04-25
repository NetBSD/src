/*	$NetBSD: sysmon.c,v 1.22 2015/04/25 22:46:31 pgoyette Exp $	*/

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
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
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Clearing house for system monitoring hardware.  We currently
 * handle environmental sensors, watchdog timers, and power management.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon.c,v 1.22 2015/04/25 22:46:31 pgoyette Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/device.h>

#include <dev/sysmon/sysmonvar.h>

dev_type_open(sysmonopen);
dev_type_close(sysmonclose);
dev_type_ioctl(sysmonioctl);
dev_type_read(sysmonread);
dev_type_poll(sysmonpoll);
dev_type_kqfilter(sysmonkqfilter);

const struct cdevsw sysmon_cdevsw = {
	.d_open = sysmonopen,
	.d_close = sysmonclose,
	.d_read = sysmonread,
	.d_write = nowrite,
	.d_ioctl = sysmonioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = sysmonpoll,
	.d_mmap = nommap,
	.d_kqfilter = sysmonkqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

static int	sysmon_match(device_t, cfdata_t, void *);
static void	sysmon_attach(device_t, device_t, void *);
static int	sysmon_detach(device_t, int);

static int	sysmon_modcmd(modcmd_t, void *); 

CFDRIVER_DECL(sysmon, DV_DULL, NULL);

/*
 * Info about our minor "devices"
 */
static struct sysmon_opvec	*sysmon_opvec_table[] = { NULL, NULL, NULL };
static int			sysmon_refcnt[] = { 0, 0, 0 };
static const char		*sysmon_mod[] = { "sysmon_envsys",
						  "sysmon_wdog",
						  "sysmon_power" };

struct sysmon_softc { 
	device_t sc_dev;
	kmutex_t sc_minor_mtx;
}; 

static device_t sysmon_dev = NULL;

CFATTACH_DECL_NEW(sysmon, sizeof(struct sysmon_softc),
        sysmon_match, sysmon_attach, sysmon_detach, NULL);
extern struct cfdriver sysmon_cd;

static int
sysmon_match(device_t parent, cfdata_t data, void *aux)   
{

	return 1;
}

static void
sysmon_attach(device_t parent, device_t self, void *aux)
{

        struct sysmon_softc *sc = device_private(self);
                    
        sc->sc_dev = self;

	mutex_init(&sc->sc_minor_mtx, MUTEX_DEFAULT, IPL_NONE);
}

static int
sysmon_detach(device_t self, int flags)
{
        struct sysmon_softc *sc = device_private(self);

	mutex_destroy(&sc->sc_minor_mtx);
	return 0;
}

/*
 * sysmon_attach_minor
 *
 *	Attach a minor device for wdog, power, or envsys.  Manage a
 *	reference count so we can prevent the device from being
 *	detached if there are still users with the minor device opened.
 *
 *	If the opvec argument is NULL, this is a request to detach the
 *	minor device - make sure the refcnt is zero!
 */
int
sysmon_attach_minor(int minor, struct sysmon_opvec *opvec)
{
	struct sysmon_softc *sc = device_private(sysmon_dev);
	int ret;

	mutex_enter(&sc->sc_minor_mtx);
	if (opvec) {
		if (sysmon_opvec_table[minor] == NULL) {
			sysmon_refcnt[minor] = 0;
			sysmon_opvec_table[minor] = opvec;
			ret = 0;
		} else
			ret = EEXIST;
	} else {
		if (sysmon_refcnt[minor] == 0) {
			sysmon_opvec_table[minor] = NULL;
			ret = 0;
		} else
			ret = EBUSY;
	}

	mutex_exit(&sc->sc_minor_mtx);
	return ret;
}

/*
 * sysmonopen:
 *
 *	Open the system monitor device.
 */
int
sysmonopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct sysmon_softc *sc = device_private(sysmon_dev);
	int error;

	mutex_enter(&sc->sc_minor_mtx);

	switch (minor(dev)) {
	case SYSMON_MINOR_ENVSYS:
	case SYSMON_MINOR_WDOG:
	case SYSMON_MINOR_POWER:
		if (sysmon_opvec_table[minor(dev)] == NULL) {
			mutex_exit(&sc->sc_minor_mtx);
			error = module_autoload(sysmon_mod[minor(dev)],
						MODULE_CLASS_MISC);
			mutex_enter(&sc->sc_minor_mtx);
			if (sysmon_opvec_table[minor(dev)] == NULL)
				error = ENODEV;
		}
		error = (sysmon_opvec_table[minor(dev)]->so_open)(dev, flag,
		    mode, l);
		if (error == 0)
			sysmon_refcnt[minor(dev)]++;
		break;
	default:
		error = ENODEV;
	}

	mutex_exit(&sc->sc_minor_mtx);
	return (error);
}

/*
 * sysmonclose:
 *
 *	Close the system monitor device.
 */
int
sysmonclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error;

	switch (minor(dev)) {
	case SYSMON_MINOR_ENVSYS:
	case SYSMON_MINOR_WDOG:
	case SYSMON_MINOR_POWER:
		if (sysmon_opvec_table[minor(dev)] == NULL)
			error = ENODEV;
		else {
			error = (sysmon_opvec_table[minor(dev)]->so_close)(dev,
			    flag, mode, l);
			if (error == 0) {
				sysmon_refcnt[minor(dev)]--;
				KASSERT(sysmon_refcnt[minor(dev)] >= 0);
			}
		}
		break;
	default:
		error = ENODEV;
	}

	return (error);
}

/*
 * sysmonioctl:
 *
 *	Perform a control request.
 */
int
sysmonioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error;

	switch (minor(dev)) {
	case SYSMON_MINOR_ENVSYS:
	case SYSMON_MINOR_WDOG:
	case SYSMON_MINOR_POWER:
		if (sysmon_opvec_table[minor(dev)] == NULL)
			error = ENODEV;
		else
			error = (sysmon_opvec_table[minor(dev)]->so_ioctl)(dev,
			    cmd, data, flag, l);
		break;
	default:
		error = ENODEV;
	}

	return (error);
}

/*
 * sysmonread:
 *
 *	Perform a read request.
 */
int
sysmonread(dev_t dev, struct uio *uio, int flags)
{
	int error;

	switch (minor(dev)) {
	case SYSMON_MINOR_POWER:
		if (sysmon_opvec_table[minor(dev)] == NULL)
			error = ENODEV;
		else
			error = (sysmon_opvec_table[minor(dev)]->so_read)(dev,
			    uio, flags);
		break;
	default:
		error = ENODEV;
	}

	return (error);
}

/*
 * sysmonpoll:
 *
 *	Poll the system monitor device.
 */
int
sysmonpoll(dev_t dev, int events, struct lwp *l)
{
	int rv;

	switch (minor(dev)) {
	case SYSMON_MINOR_POWER:
		if (sysmon_opvec_table[minor(dev)] == NULL)
			rv = events;
		else
			rv = (sysmon_opvec_table[minor(dev)]->so_poll)(dev,
			    events, l);
		break;
	default:
		rv = events;
	}

	return (rv);
}

/*
 * sysmonkqfilter:
 *
 *	Kqueue filter for the system monitor device.
 */
int
sysmonkqfilter(dev_t dev, struct knote *kn)
{
	int error;

	switch (minor(dev)) {
	case SYSMON_MINOR_POWER:
		if (sysmon_opvec_table[minor(dev)] == NULL)
			error = ENODEV;
		else
			error = (sysmon_opvec_table[minor(dev)]->so_filter)(dev,
			    kn);
		break;
	default:
		error = 1;
	}

	return (error);
}

MODULE(MODULE_CLASS_DRIVER, sysmon, "");

int
sysmon_init(void)
{
#ifdef _MODULE
	devmajor_t bmajor, cmajor;
#endif
	static struct cfdata cf;
	int error = 0;

	if (sysmon_dev != NULL) {
		return EEXIST;
	}

	error = config_cfdriver_attach(&sysmon_cd);
	if (error) {
		aprint_error("%s: unable to attach cfdriver\n",
		    sysmon_cd.cd_name);
		return error;
	}
	error = config_cfattach_attach(sysmon_cd.cd_name, &sysmon_ca);
	if (error) {
		config_cfdriver_detach(&sysmon_cd);
		aprint_error("%s: unable to attach cfattach\n",
		    sysmon_cd.cd_name);
		return error;
	}

#ifdef _MODULE
	bmajor = cmajor = -1;
	error = devsw_attach("sysmon", NULL, &bmajor,
			&sysmon_cdevsw, &cmajor);
	if (error) {
		config_cfattach_detach(sysmon_cd.cd_name, &sysmon_ca);
		config_cfdriver_detach(&sysmon_cd);
		aprint_error("%s: unable to attach devsw\n",
		    sysmon_cd.cd_name);
		return error;
	}
#endif

	cf.cf_name = sysmon_cd.cd_name;
	cf.cf_atname = sysmon_cd.cd_name; 
	cf.cf_unit = 0;
	cf.cf_fstate = FSTATE_STAR;
	cf.cf_pspec = NULL;
	cf.cf_loc = NULL;
	cf.cf_flags = 0;
 
	sysmon_dev = config_attach_pseudo(&cf);
	if (sysmon_dev == NULL) {
		aprint_error("%s: failed to attach pseudo device\n",
		    sysmon_cd.cd_name);
		error = ENODEV;
	}

	if (pmf_device_register(sysmon_dev, NULL, NULL))
		aprintf_error("%s: failed to register with pmf\n",
		    sysmon_cd.cd_name);

	return error;
}

int
sysmon_fini(void)
{
	int error = 0;

	if (sysmon_opvec_table[SYSMON_MINOR_ENVSYS] != NULL)
		error = EBUSY;
	else if (sysmon_opvec_table[SYSMON_MINOR_WDOG] != NULL)
		error = EBUSY;
	else if (sysmon_opvec_table[SYSMON_MINOR_POWER] != NULL)
		error = EBUSY;

	else {
		pmf_device_deregister(sysmon_dev);
		config_detach(sysmon_dev, 0);
		devsw_detach(NULL, &sysmon_cdevsw);
		config_cfattach_detach(sysmon_cd.cd_name, &sysmon_ca);
		config_cfdriver_detach(&sysmon_cd);
	}
	if (error == 0)
		sysmon_dev = NULL;

	return error;
}

static
int   
sysmon_modcmd(modcmd_t cmd, void *arg)
{
	int ret;
 
	switch (cmd) { 
	case MODULE_CMD_INIT:
		ret = sysmon_init();
		break;
 
	case MODULE_CMD_FINI: 
		ret = sysmon_fini(); 
		break;
 
	case MODULE_CMD_STAT:
	default: 
		ret = ENOTTY;
	}
 
	return ret;
}
