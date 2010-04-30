/*	$NetBSD: rum_at_usb.c,v 1.2.2.1 2010/04/30 14:44:27 uebayasi Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>

/*
 * rum @ usb
 *
 * handwritten device configuration.... 'nuf said
 *
 * I could convert this to use the new ioconf keyword in config,
 * except I don't have the hardware for testing anymore ...
 */

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

CFDRIVER_DECL(rum, DV_IFNET, NULL);

struct cfparent ugenhc_pspec = {
	"usbus",
	"ugenhc",
	DVUNIT_ANY
};

struct cfdata usb_cfdata[] = {
	{ "usb", "usb", 0, FSTATE_STAR, NULL, 0, &ugenhc_pspec },
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

struct cfparent usbdevif_pspec = {
	"usbdevif",
	"uhub",
	DVUNIT_ANY
};

struct cfdata rum_cfdata[] = {
	{ "rum", "rum", 0, FSTATE_STAR, NULL, 0, &usbdevif_pspec },
};

#include "rump_private.h"
#include "rump_dev_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern struct cfattach usb_ca, uhub_ca, uroothub_ca, rum_ca;

	FLAWLESSCALL(config_cfdriver_attach(&usb_cd));
	FLAWLESSCALL(config_cfattach_attach("usb", &usb_ca));
	FLAWLESSCALL(config_cfdata_attach(usb_cfdata, 0));

	FLAWLESSCALL(config_cfdriver_attach(&uhub_cd));
	FLAWLESSCALL(config_cfattach_attach("uhub", &uhub_ca));
	FLAWLESSCALL(config_cfdata_attach(uhub_cfdata, 0));

	FLAWLESSCALL(config_cfdriver_attach(&rum_cd));
	FLAWLESSCALL(config_cfattach_attach("rum", &rum_ca));
	FLAWLESSCALL(config_cfdata_attach(rum_cfdata, 0));

	FLAWLESSCALL(config_cfattach_attach("uhub", &uroothub_ca));
}
