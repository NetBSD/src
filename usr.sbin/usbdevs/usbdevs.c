/*	$NetBSD: usbdevs.c,v 1.31 2014/08/12 13:40:07 skrll Exp $	*/

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

__dead static void usage(void);
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

static void
usbdev(int f, int a, int rec)
{
	struct usb_device_info di;
	int e, p, i;

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
		printf("%s(0x%04x), %s(0x%04x), rev %s",
		       product, di.udi_productNo,
		       vendor, di.udi_vendorNo, di.udi_release);
		if (di.udi_serial[0])
			printf(", serial %s", serial);
	} else
		printf("%s, %s", product, vendor);
	printf("\n");
	if (showdevs) {
		for (i = 0; i < USB_MAX_DEVNAMES; i++)
			if (di.udi_devnames[i][0])
				printf("%*s  %s\n", indent, "",
				       di.udi_devnames[i]);
	}
	if (!rec)
		return;
	for (p = 0; p < di.udi_nports; p++) {
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
			verbose = 1;
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
