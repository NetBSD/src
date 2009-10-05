/*	$NetBSD: sd_at_scsibus_at_umass.c,v 1.2 2009/10/05 08:34:53 pooka Exp $	*/

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

/*
 * sd @ scsibus @ umass @ usb
 *
 * handwritten device configuration.... 'nuf said
 */

static const struct cfiattrdata scsicf_iattrdata = {
	"scsi", 1, {
		{ "channel", "-1", -1 },
	}
};
static const struct cfiattrdata scsibuscf_iattrdata = {
	"scsibus", 0, {
		{ NULL, NULL, 0},
	}
};
static const struct cfiattrdata *const scsibuscf_attrs[] = {
	&scsibuscf_iattrdata,
	NULL,
};
CFDRIVER_DECL(scsibus, DV_DULL, scsibuscf_attrs);
CFDRIVER_DECL(sd, DV_DISK, NULL);

static const struct cfiattrdata uroothub_iattrdata = {
	"usbroothubif", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata *const usb_attrs[] = {
	&uroothub_iattrdata,
	NULL,
};
CFDRIVER_DECL(usb, DV_DULL, usb_attrs);

static const struct cfiattrdata usbdevif_iattrdata = {
	"usbdevif", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata usbifif_iattrdata = {
	"usbifif", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata *const uhub_attrs[] = {
	&usbdevif_iattrdata,
	&usbifif_iattrdata,
	NULL,
};
CFDRIVER_DECL(uhub, DV_DULL, uhub_attrs);

static const struct cfiattrdata *const umass_attrs[] = {
	&scsicf_iattrdata,
	NULL,
};
CFDRIVER_DECL(umass, DV_DULL, umass_attrs);

struct cfparent rumpusbhc_pspec = {
	"usbus",
	"rumpusbhc",
	DVUNIT_ANY
};

struct cfdata usb_cfdata[] = {
	{ "usb", "usb", 0, FSTATE_STAR, NULL, 0, &rumpusbhc_pspec },
};

struct cfparent usb_pspec = {
	"usbroothubif",
	"usb",
	DVUNIT_ANY
};

struct cfdata uhub_cfdata[] = {
	{ "uhub", "uroothub", 0, FSTATE_STAR, NULL, 0, &usb_pspec },
};

struct cfparent usbifif_pspec = {
	"usbifif",
	"uhub",
	DVUNIT_ANY
};

struct cfparent scsi_pspec = {
	"scsi",
	NULL,
	0
};

struct cfdata umass_cfdata[] = {
	{ "umass", "umass", 0, FSTATE_STAR, NULL, 0, &usbifif_pspec },
};

int scsiloc[] = {-1,-1,-1,-1,-1,-1};

struct cfdata scsibus_cfdata[] = {
	{ "scsibus", "scsibus", 0, FSTATE_STAR, scsiloc, 0, &scsi_pspec },
};

struct cfparent scsibus_pspec = {
	"scsibus",
	"scsibus",
	DVUNIT_ANY
};

struct cfdata sd_cfdata[] = {
	{ "sd", "sd", 0, FSTATE_STAR, NULL, 0, &scsibus_pspec },
};

#include "rump_dev_private.h"
#include "rump_vfs_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

void
rump_device_configuration(void)
{
	extern struct cfattach usb_ca, uhub_ca, uroothub_ca, umass_ca;
	extern struct cfattach scsibus_ca, sd_ca;
	extern struct bdevsw sd_bdevsw;
	extern struct cdevsw sd_cdevsw;
	devmajor_t bmaj, cmaj;

	FLAWLESSCALL(config_cfdriver_attach(&usb_cd));
	FLAWLESSCALL(config_cfattach_attach("usb", &usb_ca));
	FLAWLESSCALL(config_cfdata_attach(usb_cfdata, 0));

	FLAWLESSCALL(config_cfdriver_attach(&uhub_cd));
	FLAWLESSCALL(config_cfattach_attach("uhub", &uhub_ca));
	FLAWLESSCALL(config_cfdata_attach(uhub_cfdata, 0));

	FLAWLESSCALL(config_cfdriver_attach(&umass_cd));
	FLAWLESSCALL(config_cfattach_attach("umass", &umass_ca));
	FLAWLESSCALL(config_cfdata_attach(umass_cfdata, 0));

	FLAWLESSCALL(config_cfdriver_attach(&scsibus_cd));
	FLAWLESSCALL(config_cfattach_attach("scsibus", &scsibus_ca));
	FLAWLESSCALL(config_cfdata_attach(scsibus_cfdata, 0));

	FLAWLESSCALL(config_cfdriver_attach(&sd_cd));
	FLAWLESSCALL(config_cfattach_attach("sd", &sd_ca));
	FLAWLESSCALL(config_cfdata_attach(sd_cfdata, 0));

	FLAWLESSCALL(config_cfattach_attach("uhub", &uroothub_ca));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("sd", &sd_bdevsw, &bmaj, &sd_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFBLK, "sd0", 'a', bmaj, 0, 8));
}
