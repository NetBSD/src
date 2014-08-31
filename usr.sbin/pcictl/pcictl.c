/*	$NetBSD: pcictl.c,v 1.20 2014/08/31 09:59:08 wiz Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * pcictl(8) -- a program to manipulate the PCI bus
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pci.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>

struct command {
	const char *cmd_name;
	const char *arg_names;
	void (*cmd_func)(int, char *[]);
	int open_flags;
};

__dead static void	usage(void);

static int	pcifd;

static struct pciio_businfo pci_businfo;

static const	char *dvname;
static char	dvname_store[MAXPATHLEN];
static const	char *cmdname;
static int	print_numbers = 0;
static int	print_names = 0;

static void	cmd_list(int, char *[]);
static void	cmd_dump(int, char *[]);

static const struct command commands[] = {
	{ "list",
	  "[-Nn] [-b bus] [-d device] [-f function]",
	  cmd_list,
	  O_RDONLY },

	{ "dump",
	  "[-b bus] -d device [-f function]",
	  cmd_dump,
	  O_RDONLY },

	{ 0, 0, 0, 0 },
};

static int	parse_bdf(const char *);

static void	scan_pci(int, int, int, void (*)(u_int, u_int, u_int));

static void	scan_pci_list(u_int, u_int, u_int);
static void	scan_pci_dump(u_int, u_int, u_int);

int
main(int argc, char *argv[])
{
	int i;

	/* Must have at least: device command */
	if (argc < 3)
		usage();

	/* Skip program name, get and skip device name, get command. */
	dvname = argv[1];
	cmdname = argv[2];
	argv += 2;
	argc -= 2;

	/* Look up and call the command. */
	for (i = 0; commands[i].cmd_name != NULL; i++)
		if (strcmp(cmdname, commands[i].cmd_name) == 0)
			break;
	if (commands[i].cmd_name == NULL)
		errx(EXIT_FAILURE, "unknown command: %s", cmdname);

	/* Open the device. */
	if ((strchr(dvname, '/') == NULL) &&
	    (snprintf(dvname_store, sizeof(dvname_store), _PATH_DEV "%s",
	     dvname) < (int)sizeof(dvname_store)))
		dvname = dvname_store;
	pcifd = open(dvname, commands[i].open_flags);
	if (pcifd < 0)
		err(EXIT_FAILURE, "%s", dvname);

	/* Make sure the device is a PCI bus. */
	if (ioctl(pcifd, PCI_IOC_BUSINFO, &pci_businfo) != 0)
		errx(EXIT_FAILURE, "%s: not a PCI bus device", dvname);

	(*commands[i].cmd_func)(argc, argv);
	exit(EXIT_SUCCESS);
}

static void
usage(void)
{
	int i;

	fprintf(stderr, "usage: %s device command [arg [...]]\n",
	    getprogname());

	fprintf(stderr, "   Available commands:\n");
	for (i = 0; commands[i].cmd_name != NULL; i++)
		fprintf(stderr, "\t%s %s\n", commands[i].cmd_name,
		    commands[i].arg_names);

	exit(EXIT_FAILURE);
}

static void
cmd_list(int argc, char *argv[])
{
	int bus, dev, func;
	int ch;

	bus = -1;
	dev = func = -1;

	while ((ch = getopt(argc, argv, "b:d:f:Nn")) != -1) {
		switch (ch) {
		case 'b':
			bus = parse_bdf(optarg);
			break;
		case 'd':
			dev = parse_bdf(optarg);
			break;
		case 'f':
			func = parse_bdf(optarg);
			break;
		case 'n':
			print_numbers = 1;
			break;
		case 'N':
			print_names = 1;
			break;
		default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;

	if (argc != 0)
		usage();

	scan_pci(bus, dev, func, scan_pci_list);
}

static void
cmd_dump(int argc, char *argv[])
{
	int bus, dev, func;
	int ch;

	bus = pci_businfo.busno;
	func = 0;
	dev = -1;

	while ((ch = getopt(argc, argv, "b:d:f:")) != -1) {
		switch (ch) {
		case 'b':
			bus = parse_bdf(optarg);
			break;
		case 'd':
			dev = parse_bdf(optarg);
			break;
		case 'f':
			func = parse_bdf(optarg);
			break;
		default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;

	if (argc != 0)
		usage();

	if (bus == -1)
		errx(EXIT_FAILURE, "dump: wildcard bus number not permitted");
	if (dev == -1)
		errx(EXIT_FAILURE, "dump: must specify a device number");
	if (func == -1)
		errx(EXIT_FAILURE, "dump: wildcard function number not permitted");

	scan_pci(bus, dev, func, scan_pci_dump);
}

static int
parse_bdf(const char *str)
{
	long value;
	char *end;

	if (strcmp(str, "all") == 0 ||
	    strcmp(str, "any") == 0)
		return (-1);

	value = strtol(str, &end, 0);
	if (*end != '\0') 
		errx(EXIT_FAILURE, "\"%s\" is not a number", str);

	return value;
}

static void
scan_pci(int busarg, int devarg, int funcarg, void (*cb)(u_int, u_int, u_int))
{
	u_int busmin, busmax;
	u_int devmin, devmax;
	u_int funcmin, funcmax;
	u_int bus, dev, func;
	pcireg_t id, bhlcr;

	if (busarg == -1) {
		busmin = 0;
		busmax = 255;
	} else
		busmin = busmax = busarg;

	if (devarg == -1) {
		devmin = 0;
		if (pci_businfo.maxdevs <= 0)
			devmax = 0;
		else
			devmax = pci_businfo.maxdevs - 1;
	} else
		devmin = devmax = devarg;

	for (bus = busmin; bus <= busmax; bus++) {
		for (dev = devmin; dev <= devmax; dev++) {
			if (pcibus_conf_read(pcifd, bus, dev, 0,
			    PCI_BHLC_REG, &bhlcr) != 0)
				continue;
			if (funcarg == -1) {
				funcmin = 0;
				if (PCI_HDRTYPE_MULTIFN(bhlcr))
					funcmax = 7;
				else
					funcmax = 0;
			} else
				funcmin = funcmax = funcarg;
			for (func = funcmin; func <= funcmax; func++) {
				if (pcibus_conf_read(pcifd, bus, dev,
				    func, PCI_ID_REG, &id) != 0)
					continue;

				/* Invalid vendor ID value? */
				if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
					continue;
				/*
				 * XXX Not invalid, but we've done this
				 * ~forever.
				 */
				if (PCI_VENDOR(id) == 0)
					continue;

				(*cb)(bus, dev, func);
			}
		}
	}
}

static void
scan_pci_list(u_int bus, u_int dev, u_int func)
{
	pcireg_t id, class;
	char devinfo[256];

	if (pcibus_conf_read(pcifd, bus, dev, func, PCI_ID_REG, &id) != 0)
		return;
	if (pcibus_conf_read(pcifd, bus, dev, func, PCI_CLASS_REG, &class) != 0)
		return;

	printf("%03u:%02u:%01u: ", bus, dev, func);
	if (print_numbers) {
		printf("0x%08x (0x%08x)", id, class);
	} else {
		pci_devinfo(id, class, 1, devinfo, sizeof(devinfo));
		printf("%s", devinfo);
	}
	if (print_names) {
		char drvname[16];
		if (pci_drvname(pcifd, dev, func, drvname, sizeof drvname) == 0)
			printf(" [%s]", drvname);
	}
	printf("\n");
}

static void
scan_pci_dump(u_int bus, u_int dev, u_int func)
{

	pci_conf_print(pcifd, bus, dev, func);
}
