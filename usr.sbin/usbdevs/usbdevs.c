/*	$NetBSD: usbdevs.c,v 1.35.12.1 2018/07/28 04:38:15 pgoyette Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org).
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: usbdevs.c,v 1.35.12.1 2018/07/28 04:38:15 pgoyette Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <locale.h>
#include <langinfo.h>
#include <iconv.h>
#include <dev/usb/usb.h>

#define USBDEV "/dev/usb"

static int verbose = 0;
static int showdevs = 0;

struct stringtable {
	int row, col;
	const char *string;
};

__dead static void usage(void);
static void getstrings(const struct stringtable *, int, int, const char **, const char **);
static void usbdev(int f, int a, int rec);
static void usbdump(int f);
static void dumpone(char *name, int f, int addr);

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-dv] [-a addr] [-f dev]\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static char done[USB_MAX_DEVICES];
static int indent;
#define MAXLEN USB_MAX_ENCODED_STRING_LEN /* assume can't grow over UTF-8 */
static char vendor[MAXLEN], product[MAXLEN], serial[MAXLEN];

static void
u2t(const char *utf8str, char *termstr)
{
	static iconv_t ic;
	static int iconv_inited = 0;
	size_t insz, outsz, icres;

	if (!iconv_inited) {
		setlocale(LC_ALL, "");
		ic = iconv_open(nl_langinfo(CODESET), "UTF-8");
		if (ic == (iconv_t)-1)
			ic = iconv_open("ASCII", "UTF-8"); /* g.c.d. */
		iconv_inited = 1;
	}
	if (ic != (iconv_t)-1) {
		insz = strlen(utf8str);
		outsz = MAXLEN - 1;
		icres = iconv(ic, &utf8str, &insz, &termstr, &outsz);
		if (icres != (size_t)-1) {
			*termstr = '\0';
			return;
		}
	}
	strcpy(termstr, "(invalid)");
}

struct stringtable class_strings[] = {
	{ UICLASS_UNSPEC,      -1, "Unspecified" },

	{ UICLASS_AUDIO,       -1, "Audio" },
	{ UICLASS_AUDIO,       UISUBCLASS_AUDIOCONTROL, "Audio Control" },
	{ UICLASS_AUDIO,       UISUBCLASS_AUDIOSTREAM, "Audio Streaming" },
	{ UICLASS_AUDIO,       UISUBCLASS_MIDISTREAM, "MIDI Streaming" },

	{ UICLASS_CDC,         -1, "Communications and CDC Control" },
	{ UICLASS_CDC,         UISUBCLASS_DIRECT_LINE_CONTROL_MODEL, "Direct Line" },
	{ UICLASS_CDC,         UISUBCLASS_ABSTRACT_CONTROL_MODEL, "Abstract" },
	{ UICLASS_CDC,         UISUBCLASS_TELEPHONE_CONTROL_MODEL, "Telephone" },
	{ UICLASS_CDC,         UISUBCLASS_MULTICHANNEL_CONTROL_MODEL, "Multichannel" },
	{ UICLASS_CDC,         UISUBCLASS_CAPI_CONTROLMODEL, "CAPI" },
	{ UICLASS_CDC,         UISUBCLASS_ETHERNET_NETWORKING_CONTROL_MODEL, "Ethernet Networking" },
	{ UICLASS_CDC,         UISUBCLASS_ATM_NETWORKING_CONTROL_MODEL, "ATM Networking" },

	{ UICLASS_HID,         -1, "Human Interface Device" },
	{ UICLASS_HID,         UISUBCLASS_BOOT, "Boot" },

	{ UICLASS_PHYSICAL,    -1, "Physical" },

	{ UICLASS_IMAGE,       -1, "Image" },

	{ UICLASS_PRINTER,     -1, "Printer" },
	{ UICLASS_PRINTER,     UISUBCLASS_PRINTER, "Printer" },

	{ UICLASS_MASS,        -1, "Mass Storage" },
	{ UICLASS_MASS,        UISUBCLASS_RBC, "RBC" },
	{ UICLASS_MASS,        UISUBCLASS_SFF8020I, "SFF8020I" },
	{ UICLASS_MASS,        UISUBCLASS_QIC157, "QIC157" },
	{ UICLASS_MASS,        UISUBCLASS_UFI, "UFI" },
	{ UICLASS_MASS,        UISUBCLASS_SFF8070I, "SFF8070I" },
	{ UICLASS_MASS,        UISUBCLASS_SCSI, "SCSI" },
	{ UICLASS_MASS,        UISUBCLASS_SCSI, "SCSI" },

	{ UICLASS_HUB,         -1, "Hub" },
	{ UICLASS_HUB,         UISUBCLASS_HUB, "Hub" },

	{ UICLASS_CDC_DATA,    -1, "CDC-Data" },
	{ UICLASS_CDC_DATA,    UISUBCLASS_DATA, "Data" },

	{ UICLASS_SMARTCARD,   -1, "Smart Card" },

	{ UICLASS_SECURITY,    -1, "Content Security" },

	{ UICLASS_VIDEO,       -1, "Video" },
	{ UICLASS_VIDEO,       UISUBCLASS_VIDEOCONTROL, "Video Control" },
	{ UICLASS_VIDEO,       UISUBCLASS_VIDEOSTREAMING, "Video Streaming" },
	{ UICLASS_VIDEO,       UISUBCLASS_VIDEOCOLLECTION, "Video Collection" },

#ifdef notyet
	{ UICLASS_HEALTHCARE,  -1, "Personal Healthcare" },
	{ UICLASS_AVDEVICE,    -1, "Audio/Video Device" },
	{ UICLASS_BILLBOARD,   -1, "Billboard" },
#endif

	{ UICLASS_DIAGNOSTIC,  -1, "Diagnostic" },
	{ UICLASS_WIRELESS,    -1, "Wireless" },
	{ UICLASS_WIRELESS,    UISUBCLASS_RF, "Radio Frequency" },

#ifdef notyet
	{ UICLASS_MISC,        -1, "Miscellaneous" },
#endif

	{ UICLASS_APPL_SPEC,   -1, "Application Specific" },
	{ UICLASS_APPL_SPEC,   UISUBCLASS_FIRMWARE_DOWNLOAD, "Firmware Download" },
	{ UICLASS_APPL_SPEC,   UISUBCLASS_IRDA,              "Irda" },

	{ UICLASS_VENDOR,      -1, "Vendor Specific" },

	{ -1, -1, NULL }
};

static void
getstrings(const struct stringtable *table,
           int row, int col, const char **rp, const char **cp) {
	static char rbuf[5], cbuf[5];

	snprintf(rbuf, sizeof(rbuf), "0x%02x", row);
	snprintf(cbuf, sizeof(cbuf), "0x%02x", col);

	*rp = rbuf;
	*cp = cbuf;

	while (table->string != NULL) {
		if (table->row == row) {
			if (table->col == -1)
				*rp = table->string;
			else if (table->col == col)
				*cp = table->string;
		} else if (table->row > row)
			break;

		++table;
	}
}

static void
usbdev(int f, int a, int rec)
{
	struct usb_device_info di;
	int e, i;

	di.udi_addr = a;
	e = ioctl(f, USB_DEVICEINFO, &di);
	if (e) {
		if (errno != ENXIO)
			printf("addr %d: I/O error\n", a);
		return;
	}
	printf("addr %d: ", a);
	done[a] = 1;
	if (verbose) {
		switch (di.udi_speed) {
		case USB_SPEED_LOW:  printf("low speed, "); break;
		case USB_SPEED_FULL: printf("full speed, "); break;
		case USB_SPEED_HIGH: printf("high speed, "); break;
		case USB_SPEED_SUPER: printf("super speed, "); break;
		case USB_SPEED_SUPER_PLUS: printf("super speed+, "); break;
		default: break;
		}
		if (di.udi_power)
			printf("power %d mA, ", di.udi_power);
		else
			printf("self powered, ");
		if (di.udi_config)
			printf("config %d, ", di.udi_config);
		else
			printf("unconfigured, ");
	}
	u2t(di.udi_product, product);
	u2t(di.udi_vendor, vendor);
	u2t(di.udi_serial, serial);
	if (verbose) {
		printf("%s(0x%04x), %s(0x%04x), rev %s(0x%04x)",
		       product, di.udi_productNo,
		       vendor, di.udi_vendorNo,
			di.udi_release, di.udi_releaseNo);
		if (di.udi_serial[0])
			printf(", serial %s", serial);
	} else
		printf("%s, %s", product, vendor);
	printf("\n");
	if (verbose > 1 && di.udi_class != UICLASS_UNSPEC) {
		const char *cstr, *sstr;
		getstrings(class_strings, di.udi_class, di.udi_subclass, &cstr, &sstr);
		printf("%*s  %s(0x%02x), %s(0x%02x), proto %u\n", indent, "",
			cstr, di.udi_class, sstr, di.udi_subclass,
			di.udi_protocol);
	}
	if (showdevs) {
		for (i = 0; i < USB_MAX_DEVNAMES; i++)
			if (di.udi_devnames[i][0])
				printf("%*s  %s\n", indent, "",
				       di.udi_devnames[i]);
	}
	if (!rec)
		return;

	unsigned int nports = di.udi_nports;

	for (unsigned int p = 0; p < nports && p < __arraycount(di.udi_ports); p++) {
		int s = di.udi_ports[p];
		if (s >= USB_MAX_DEVICES) {
			if (verbose) {
				printf("%*sport %d %s\n", indent+1, "", p+1,
				       s == USB_PORT_ENABLED ? "enabled" :
				       s == USB_PORT_SUSPENDED ? "suspended" : 
				       s == USB_PORT_POWERED ? "powered" :
				       s == USB_PORT_DISABLED ? "disabled" :
				       "???");
				
			}
			continue;
		}
		indent++;
		printf("%*s", indent, "");
		if (verbose)
			printf("port %d ", p+1);
		if (s == 0)
			printf("addr 0 should never happen!\n");
		else
			usbdev(f, s, 1);
		indent--;
	}
}

static void
usbdump(int f)
{
	int a;

	for (a = 0; a < USB_MAX_DEVICES; a++) {
		if (!done[a])
			usbdev(f, a, 1);
	}
}

static void
dumpone(char *name, int f, int addr)
{
	if (verbose)
		printf("Controller %s:\n", name);
	indent = 0;
	memset(done, 0, sizeof done);
	if (addr >= 0)
		usbdev(f, addr, 0);
	else
		usbdump(f);
}

int
main(int argc, char **argv)
{
	int ch, i, f;
	char buf[50];
	char *dev = NULL;
	int addr = -1;
	int ncont;

	while ((ch = getopt(argc, argv, "a:df:v?")) != -1) {
		switch(ch) {
		case 'a':
			addr = atoi(optarg);
			break;
		case 'd':
			showdevs++;
			break;
		case 'f':
			dev = optarg;
			break;
		case 'v':
			verbose++;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (dev == NULL) {
		for (ncont = 0, i = 0; i < 10; i++) {
			snprintf(buf, sizeof(buf), "%s%d", USBDEV, i);
			f = open(buf, O_RDONLY);
			if (f >= 0) {
				dumpone(buf, f, addr);
				close(f);
			} else {
				if (errno == ENOENT || errno == ENXIO)
					continue;
				warn("%s", buf);
			}
			ncont++;
		}
		if (verbose && ncont == 0)
			printf("%s: no USB controllers found\n",
			    getprogname());
	} else {
		f = open(dev, O_RDONLY);
		if (f >= 0)
			dumpone(dev, f, addr);
		else
			err(1, "%s", dev);
	}
	exit(EXIT_SUCCESS);
}
