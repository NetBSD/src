/*	$NetBSD: ucom_at_usb.c,v 1.2 2009/12/20 19:44:21 pooka Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>

/*
 * MACHINE GENERATED: DO NOT EDIT
 *
 * ioconf.c, from "TESTI_ucom"
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>

static const struct cfiattrdata gpibdevcf_iattrdata = {
	"gpibdev", 1,
	{
		{ "address", "-1", -1 },
	}
};
static const struct cfiattrdata acpibuscf_iattrdata = {
	"acpibus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata caccf_iattrdata = {
	"cac", 1,
	{
		{ "unit", "-1", -1 },
	}
};
static const struct cfiattrdata spicf_iattrdata = {
	"spi", 1,
	{
		{ "slave", "NULL", 0 },
	}
};
static const struct cfiattrdata radiodevcf_iattrdata = {
	"radiodev", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata mlxcf_iattrdata = {
	"mlx", 1,
	{
		{ "unit", "-1", -1 },
	}
};
static const struct cfiattrdata ucombuscf_iattrdata = {
	"ucombus", 1,
	{
		{ "portno", "-1", -1 },
	}
};
static const struct cfiattrdata videobuscf_iattrdata = {
	"videobus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata isabuscf_iattrdata = {
	"isabus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata i2cbuscf_iattrdata = {
	"i2cbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata ata_hlcf_iattrdata = {
	"ata_hl", 1,
	{
		{ "drive", "-1", -1 },
	}
};
static const struct cfiattrdata depcacf_iattrdata = {
	"depca", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata ppbuscf_iattrdata = {
	"ppbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata eisabuscf_iattrdata = {
	"eisabus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata atapicf_iattrdata = {
	"atapi", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata usbroothubifcf_iattrdata = {
	"usbroothubif", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata altmemdevcf_iattrdata = {
	"altmemdev", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata tcbuscf_iattrdata = {
	"tcbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata onewirebuscf_iattrdata = {
	"onewirebus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata gpiocf_iattrdata = {
	"gpio", 2,
	{
		{ "offset", "-1", -1 },
		{ "mask", "0", 0 },
	}
};
static const struct cfiattrdata cbbuscf_iattrdata = {
	"cbbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata gpiobuscf_iattrdata = {
	"gpiobus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata drmcf_iattrdata = {
	"drm", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata pckbportcf_iattrdata = {
	"pckbport", 1,
	{
		{ "slot", "-1", -1 },
	}
};
static const struct cfiattrdata irbuscf_iattrdata = {
	"irbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata aaccf_iattrdata = {
	"aac", 1,
	{
		{ "unit", "-1", -1 },
	}
};
static const struct cfiattrdata pcibuscf_iattrdata = {
	"pcibus", 1,
	{
		{ "bus", "-1", -1 },
	}
};
static const struct cfiattrdata usbififcf_iattrdata = {
	"usbifif", 6,
	{
		{ "port", "-1", -1 },
		{ "configuration", "-1", -1 },
		{ "interface", "-1", -1 },
		{ "vendor", "-1", -1 },
		{ "product", "-1", -1 },
		{ "release", "-1", -1 },
	}
};
static const struct cfiattrdata upccf_iattrdata = {
	"upc", 1,
	{
		{ "offset", "-1", -1 },
	}
};
static const struct cfiattrdata iiccf_iattrdata = {
	"iic", 2,
	{
		{ "addr", "-1", -1 },
		{ "size", "-1", -1 },
	}
};
static const struct cfiattrdata onewirecf_iattrdata = {
	"onewire", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata mcabuscf_iattrdata = {
	"mcabus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wsdisplaydevcf_iattrdata = {
	"wsdisplaydev", 1,
	{
		{ "kbdmux", "1", 1 },
	}
};
static const struct cfiattrdata miicf_iattrdata = {
	"mii", 1,
	{
		{ "phy", "-1", -1 },
	}
};
static const struct cfiattrdata cpcbuscf_iattrdata = {
	"cpcbus", 2,
	{
		{ "addr", "NULL", 0 },
		{ "irq", "-1", -1 },
	}
};
static const struct cfiattrdata parportcf_iattrdata = {
	"parport", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata dbcoolcf_iattrdata = {
	"dbcool", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata usbdevifcf_iattrdata = {
	"usbdevif", 6,
	{
		{ "port", "-1", -1 },
		{ "configuration", "-1", -1 },
		{ "interface", "-1", -1 },
		{ "vendor", "-1", -1 },
		{ "product", "-1", -1 },
		{ "release", "-1", -1 },
	}
};
static const struct cfiattrdata wskbddevcf_iattrdata = {
	"wskbddev", 2,
	{
		{ "console", "-1", -1 },
		{ "mux", "1", 1 },
	}
};
static const struct cfiattrdata audiobuscf_iattrdata = {
	"audiobus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata btbuscf_iattrdata = {
	"btbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata midibuscf_iattrdata = {
	"midibus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata vmebuscf_iattrdata = {
	"vmebus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wsemuldisplaydevcf_iattrdata = {
	"wsemuldisplaydev", 2,
	{
		{ "console", "-1", -1 },
		{ "kbdmux", "1", 1 },
	}
};
static const struct cfiattrdata uhidbuscf_iattrdata = {
	"uhidbus", 1,
	{
		{ "reportid", "-1", -1 },
	}
};
static const struct cfiattrdata icpcf_iattrdata = {
	"icp", 1,
	{
		{ "unit", "-1", -1 },
	}
};
static const struct cfiattrdata sdmmcbuscf_iattrdata = {
	"sdmmcbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata comcf_iattrdata = {
	"com", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata spiflashbuscf_iattrdata = {
	"spiflashbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata fwbuscf_iattrdata = {
	"fwbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata pcmciaslotcf_iattrdata = {
	"pcmciaslot", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata usbuscf_iattrdata = {
	"usbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wsmousedevcf_iattrdata = {
	"wsmousedev", 1,
	{
		{ "mux", "0", 0 },
	}
};
static const struct cfiattrdata scsicf_iattrdata = {
	"scsi", 1,
	{
		{ "channel", "-1", -1 },
	}
};
static const struct cfiattrdata atacf_iattrdata = {
	"ata", 1,
	{
		{ "channel", "-1", -1 },
	}
};
static const struct cfiattrdata spibuscf_iattrdata = {
	"spibus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata pcmciabuscf_iattrdata = {
	"pcmciabus", 2,
	{
		{ "controller", "-1", -1 },
		{ "socket", "-1", -1 },
	}
};

static const struct cfiattrdata * const usb_attrs[] = { &usbroothubifcf_iattrdata, NULL };
CFDRIVER_DECL(usb, DV_DULL, usb_attrs);

static const struct cfiattrdata * const uhub_attrs[] = { &usbififcf_iattrdata, &usbdevifcf_iattrdata, NULL };
CFDRIVER_DECL(uhub, DV_DULL, uhub_attrs);

CFDRIVER_DECL(ucom, DV_DULL, NULL);

static const struct cfiattrdata * const uplcom_attrs[] = { &ucombuscf_iattrdata, NULL };
CFDRIVER_DECL(uplcom, DV_DULL, uplcom_attrs);

static const struct cfiattrdata * const rumpusbhc_attrs[] = { &usbuscf_iattrdata, NULL };
CFDRIVER_DECL(rumpusbhc, DV_DULL, rumpusbhc_attrs);



extern struct cfattach usb_ca;
extern struct cfattach uroothub_ca;
extern struct cfattach ucom_ca;
extern struct cfattach uplcom_ca;
extern struct cfattach rumpusbhc_ca;

/* locators */
static int loc[7] = {
	-1, -1, -1, -1, -1, -1, -1,
};

static const struct cfparent pspec1 = {
	"usbus", "rumpusbhc", DVUNIT_ANY
};
static const struct cfparent pspec2 = {
	"usbroothubif", "usb", DVUNIT_ANY
};
static const struct cfparent pspec3 = {
	"usbdevif", "uhub", DVUNIT_ANY
};
static const struct cfparent pspec4 = {
	"ucombus", "uplcom", DVUNIT_ANY
};

#define NORM FSTATE_NOTFOUND
#define STAR FSTATE_STAR

struct cfdata cfdata_ucom[] = {
    /* driver           attachment    unit state loc   flags pspec */
/*  0: usb* at rumpusbhc? */
    { "usb",		"usb",		 0, STAR,     loc,      0, &pspec1 },
/*  1: uhub* at usb? */
    { "uhub",		"uroothub",	 0, STAR,     loc,      0, &pspec2 },
/*  2: ucom* at uplcom? portno -1 */
    { "ucom",		"ucom",		 0, STAR, loc+  6,      0, &pspec4 },
/*  3: uplcom* at uhub? port -1 configuration -1 interface -1 vendor -1 product -1 release -1 */
    { "uplcom",		"uplcom",	 0, STAR, loc+  0,      0, &pspec3 },
    { NULL,		NULL,		 0, 0,    NULL,      0, NULL }
};

#include <sys/stat.h>

#include "rump_dev_private.h"
#include "rump_vfs_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

void tty_init(void);

void
rump_device_configuration(void)
{
	extern struct cfattach usb_ca, uhub_ca, uroothub_ca, ucom_ca, uplcom_ca;
	extern struct cdevsw ucom_cdevsw;
	devmajor_t cmaj, bmaj;

	FLAWLESSCALL(config_cfdata_attach(cfdata_ucom, 0));

	FLAWLESSCALL(config_cfdriver_attach(&usb_cd));
	FLAWLESSCALL(config_cfattach_attach("usb", &usb_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uhub_cd));
	FLAWLESSCALL(config_cfattach_attach("uhub", &uhub_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uplcom_cd));
	FLAWLESSCALL(config_cfattach_attach("uplcom", &uplcom_ca));

	FLAWLESSCALL(config_cfdriver_attach(&ucom_cd));
	FLAWLESSCALL(config_cfattach_attach("ucom", &ucom_ca));

	FLAWLESSCALL(config_cfattach_attach("uhub", &uroothub_ca));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("ucom", NULL, &bmaj, &ucom_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/ttyU", '0',
	    cmaj, 0, 1));
	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/dtyU", '0',
	    cmaj, 0x80000, 1));

	tty_init();
}
