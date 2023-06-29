#include "ioconf.h"
/*
 * MACHINE GENERATED: DO NOT EDIT
 *
 * ioconf.c, from "/Users/sidqian/Downloads/summer/L2S/netbsd-src/sys/arch/luna68k/conf/TESTING"
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>

static const struct cfiattrdata wskbddevcf_iattrdata = {
	"wskbddev", 2, {
		{ "console", "-1", -1 },
		{ "mux", "1", 1 },
	}
};
static const struct cfiattrdata mainbuscf_iattrdata = {
	"mainbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wsemuldisplaydevcf_iattrdata = {
	"wsemuldisplaydev", 2, {
		{ "console", "-1", -1 },
		{ "kbdmux", "1", 1 },
	}
};
static const struct cfiattrdata scsicf_iattrdata = {
	"scsi", 1, {
		{ "channel", "-1", -1 },
	}
};
static const struct cfiattrdata wsmousedevcf_iattrdata = {
	"wsmousedev", 1, {
		{ "mux", "0", 0 },
	}
};
static const struct cfiattrdata siocf_iattrdata = {
	"sio", 1, {
		{ "channel", "-1", -1 },
	}
};
static const struct cfiattrdata scsibuscf_iattrdata = {
	"scsibus", 2, {
		{ "target", "-1", -1 },
		{ "lun", "-1", -1 },
	}
};
static const struct cfiattrdata audiocf_iattrdata = {
	"audio", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata audiobuscf_iattrdata = {
	"audiobus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata xpbuscf_iattrdata = {
	"xpbus", 0, {
		{ NULL, NULL, 0 },
	}
};

CFDRIVER_DECL(swwdog, DV_DULL, NULL);

static const struct cfiattrdata * const audio_attrs[] = { &audiocf_iattrdata, NULL };
CFDRIVER_DECL(audio, DV_AUDIODEV, audio_attrs);

CFDRIVER_DECL(le, DV_IFNET, NULL);

static const struct cfiattrdata * const spc_attrs[] = { &scsicf_iattrdata, NULL };
CFDRIVER_DECL(spc, DV_DULL, spc_attrs);

CFDRIVER_DECL(wsdisplay, DV_DULL, NULL);

CFDRIVER_DECL(wskbd, DV_DULL, NULL);

CFDRIVER_DECL(wsmouse, DV_DULL, NULL);

CFDRIVER_DECL(md, DV_DISK, NULL);

CFDRIVER_DECL(fss, DV_DISK, NULL);

static const struct cfiattrdata * const mainbus_attrs[] = { &mainbuscf_iattrdata, NULL };
CFDRIVER_DECL(mainbus, DV_DULL, mainbus_attrs);

CFDRIVER_DECL(clock, DV_DULL, NULL);

CFDRIVER_DECL(lcd, DV_DULL, NULL);

static const struct cfiattrdata * const sio_attrs[] = { &siocf_iattrdata, NULL };
CFDRIVER_DECL(sio, DV_DULL, sio_attrs);

CFDRIVER_DECL(siotty, DV_TTY, NULL);

static const struct cfiattrdata * const xpbus_attrs[] = { &xpbuscf_iattrdata, NULL };
CFDRIVER_DECL(xpbus, DV_DULL, xpbus_attrs);

CFDRIVER_DECL(xp, DV_DULL, NULL);

static const struct cfiattrdata * const psgpam_attrs[] = { &audiobuscf_iattrdata, NULL };
CFDRIVER_DECL(psgpam, DV_DULL, psgpam_attrs);

static const struct cfiattrdata * const ws_attrs[] = { &wsmousedevcf_iattrdata, &wskbddevcf_iattrdata, NULL };
CFDRIVER_DECL(ws, DV_DULL, ws_attrs);

static const struct cfiattrdata * const fb_attrs[] = { &wsemuldisplaydevcf_iattrdata, NULL };
CFDRIVER_DECL(fb, DV_DULL, fb_attrs);

static const struct cfiattrdata * const scsibus_attrs[] = { &scsibuscf_iattrdata, NULL };
CFDRIVER_DECL(scsibus, DV_DULL, scsibus_attrs);

CFDRIVER_DECL(cd, DV_DISK, NULL);

CFDRIVER_DECL(sd, DV_DISK, NULL);

CFDRIVER_DECL(st, DV_TAPE, NULL);


struct cfdriver * const cfdriver_list_initial[] = {
	&swwdog_cd,
	&audio_cd,
	&le_cd,
	&spc_cd,
	&wsdisplay_cd,
	&wskbd_cd,
	&wsmouse_cd,
	&md_cd,
	&fss_cd,
	&mainbus_cd,
	&clock_cd,
	&lcd_cd,
	&sio_cd,
	&siotty_cd,
	&xpbus_cd,
	&xp_cd,
	&psgpam_cd,
	&ws_cd,
	&fb_cd,
	&scsibus_cd,
	&cd_cd,
	&sd_cd,
	&st_cd,
	NULL
};

extern struct cfattach audio_ca;
extern struct cfattach wsdisplay_emul_ca;
extern struct cfattach wskbd_ca;
extern struct cfattach wsmouse_ca;
extern struct cfattach mainbus_ca;
extern struct cfattach clock_ca;
extern struct cfattach lcd_ca;
extern struct cfattach le_ca;
extern struct cfattach sio_ca;
extern struct cfattach siotty_ca;
extern struct cfattach xpbus_ca;
extern struct cfattach xp_ca;
extern struct cfattach psgpam_ca;
extern struct cfattach ws_ca;
extern struct cfattach fb_ca;
extern struct cfattach spc_ca;
extern struct cfattach scsibus_ca;
extern struct cfattach cd_ca;
extern struct cfattach sd_ca;
extern struct cfattach st_scsibus_ca;

/* locators */
static int loc[14] = {
	-1, 1, -1, 1, -1, -1, -1, -1,
	-1, -1, 0, -1, -1, -1,
};

static const struct cfparent pspec0 = {
	"mainbus", "mainbus", 0
};
static const struct cfparent pspec1 = {
	"sio", "sio", 0
};
static const struct cfparent pspec2 = {
	"xpbus", "xpbus", 0
};
static const struct cfparent pspec3 = {
	"audiobus", "psgpam", DVUNIT_ANY
};
static const struct cfparent pspec4 = {
	"wsemuldisplaydev", "fb", DVUNIT_ANY
};
static const struct cfparent pspec5 = {
	"wskbddev", "ws", DVUNIT_ANY
};
static const struct cfparent pspec6 = {
	"wsmousedev", "ws", DVUNIT_ANY
};
static const struct cfparent pspec7 = {
	"scsi", "spc", DVUNIT_ANY
};
static const struct cfparent pspec8 = {
	"scsibus", "scsibus", DVUNIT_ANY
};

#define NORM FSTATE_NOTFOUND
#define STAR FSTATE_STAR

struct cfdata cfdata[] = {
    /* driver           attachment    unit state      loc   flags  pspec */
/*  0: audio* at psgpam? */
    { "audio",		"audio",	 0, STAR,    NULL,      0, &pspec3 },
/*  1: le0 at mainbus0 */
    { "le",		"le",		 0, NORM,    NULL,      0, &pspec0 },
/*  2: spc0 at mainbus0 */
    { "spc",		"spc",		 0, NORM,    NULL,      0, &pspec0 },
/*  3: spc1 at mainbus0 */
    { "spc",		"spc",		 1, NORM,    NULL,      0, &pspec0 },
/*  4: wsdisplay* at fb? console -1 kbdmux 1 */
    { "wsdisplay",	"wsdisplay_emul",	 0, STAR, loc+  2,      0, &pspec4 },
/*  5: wskbd* at ws? console -1 mux 1 */
    { "wskbd",		"wskbd",	 0, STAR, loc+  0,      0, &pspec5 },
/*  6: wsmouse* at ws? mux 0 */
    { "wsmouse",	"wsmouse",	 0, STAR, loc+ 10,      0, &pspec6 },
/*  7: mainbus0 at root */
    { "mainbus",	"mainbus",	 0, NORM,    NULL,      0, NULL },
/*  8: clock0 at mainbus0 */
    { "clock",		"clock",	 0, NORM,    NULL,      0, &pspec0 },
/*  9: lcd0 at mainbus0 */
    { "lcd",		"lcd",		 0, NORM,    NULL,      0, &pspec0 },
/* 10: sio0 at mainbus0 */
    { "sio",		"sio",		 0, NORM,    NULL,      0, &pspec0 },
/* 11: siotty0 at sio0 channel -1 */
    { "siotty",		"siotty",	 0, NORM, loc+ 13,      0, &pspec1 },
/* 12: xpbus0 at mainbus0 */
    { "xpbus",		"xpbus",	 0, NORM,    NULL,      0, &pspec0 },
/* 13: xp0 at xpbus0 */
    { "xp",		"xp",		 0, NORM,    NULL,      0, &pspec2 },
/* 14: psgpam0 at xpbus0 */
    { "psgpam",		"psgpam",	 0, NORM,    NULL,      0, &pspec2 },
/* 15: ws0 at sio0 channel -1 */
    { "ws",		"ws",		 0, NORM, loc+ 12,      0, &pspec1 },
/* 16: fb0 at mainbus0 */
    { "fb",		"fb",		 0, NORM,    NULL,      0, &pspec0 },
/* 17: scsibus* at spc? channel -1 */
    { "scsibus",	"scsibus",	 0, STAR, loc+ 11,      0, &pspec7 },
/* 18: cd* at scsibus? target -1 lun -1 */
    { "cd",		"cd",		 0, STAR, loc+  4,      0, &pspec8 },
/* 19: sd* at scsibus? target -1 lun -1 */
    { "sd",		"sd",		 0, STAR, loc+  6,      0, &pspec8 },
/* 20: st* at scsibus? target -1 lun -1 */
    { "st",		"st_scsibus",	 0, STAR, loc+  8,      0, &pspec8 },
    { NULL,		NULL,		 0,    0,    NULL,      0, NULL }
};

static struct cfattach * const audio_cfattachinit[] = {
	&audio_ca, NULL
};
static struct cfattach * const le_cfattachinit[] = {
	&le_ca, NULL
};
static struct cfattach * const spc_cfattachinit[] = {
	&spc_ca, NULL
};
static struct cfattach * const wsdisplay_cfattachinit[] = {
	&wsdisplay_emul_ca, NULL
};
static struct cfattach * const wskbd_cfattachinit[] = {
	&wskbd_ca, NULL
};
static struct cfattach * const wsmouse_cfattachinit[] = {
	&wsmouse_ca, NULL
};
static struct cfattach * const mainbus_cfattachinit[] = {
	&mainbus_ca, NULL
};
static struct cfattach * const clock_cfattachinit[] = {
	&clock_ca, NULL
};
static struct cfattach * const lcd_cfattachinit[] = {
	&lcd_ca, NULL
};
static struct cfattach * const sio_cfattachinit[] = {
	&sio_ca, NULL
};
static struct cfattach * const siotty_cfattachinit[] = {
	&siotty_ca, NULL
};
static struct cfattach * const xpbus_cfattachinit[] = {
	&xpbus_ca, NULL
};
static struct cfattach * const xp_cfattachinit[] = {
	&xp_ca, NULL
};
static struct cfattach * const psgpam_cfattachinit[] = {
	&psgpam_ca, NULL
};
static struct cfattach * const ws_cfattachinit[] = {
	&ws_ca, NULL
};
static struct cfattach * const fb_cfattachinit[] = {
	&fb_ca, NULL
};
static struct cfattach * const scsibus_cfattachinit[] = {
	&scsibus_ca, NULL
};
static struct cfattach * const cd_cfattachinit[] = {
	&cd_ca, NULL
};
static struct cfattach * const sd_cfattachinit[] = {
	&sd_ca, NULL
};
static struct cfattach * const st_cfattachinit[] = {
	&st_scsibus_ca, NULL
};

const struct cfattachinit cfattachinit[] = {
	{ "audio", audio_cfattachinit },
	{ "le", le_cfattachinit },
	{ "spc", spc_cfattachinit },
	{ "wsdisplay", wsdisplay_cfattachinit },
	{ "wskbd", wskbd_cfattachinit },
	{ "wsmouse", wsmouse_cfattachinit },
	{ "mainbus", mainbus_cfattachinit },
	{ "clock", clock_cfattachinit },
	{ "lcd", lcd_cfattachinit },
	{ "sio", sio_cfattachinit },
	{ "siotty", siotty_cfattachinit },
	{ "xpbus", xpbus_cfattachinit },
	{ "xp", xp_cfattachinit },
	{ "psgpam", psgpam_cfattachinit },
	{ "ws", ws_cfattachinit },
	{ "fb", fb_cfattachinit },
	{ "scsibus", scsibus_cfattachinit },
	{ "cd", cd_cfattachinit },
	{ "sd", sd_cfattachinit },
	{ "st", st_cfattachinit },
	{ NULL, NULL }
};

const short cfroots[] = {
	 7 /* mainbus0 */,
	-1
};

/* pseudo-devices */

const struct pdevinit pdevinit[] = {
	{ cpuctlattach, 1 },
	{ rndattach, 1 },
	{ bpfilterattach, 1 },
	{ loopattach, 1 },
	{ mdattach, 1 },
	{ ptyattach, 1 },
	{ fssattach, 1 },
	{ vlanattach, 1 },
	{ bridgeattach, 1 },
	{ agrattach, 1 },
	{ clockctlattach, 1 },
	{ drvctlattach, 1 },
	{ ksymsattach, 1 },
	{ swwdogattach, 1 },
	{ wsmuxattach, 1 },
	{ 0, 0 }
};
