/*	$NetBSD: scsictl.c,v 1.7.2.1 1999/12/27 18:30:33 wrstuden Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * scsictl(8) - a program to manipulate SCSI devices and busses.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsipiconf.h>

#include "extern.h"

struct command {
	const char *cmd_name;
	const char *arg_names;
	void (*cmd_func) __P((int, char *[]));
};

int	main __P((int, char *[]));
void	usage __P((void));

int	fd;				/* file descriptor for device */
const	char *dvname;			/* device name */
char	dvname_store[MAXPATHLEN];	/* for opendisk(3) */
const	char *cmdname;			/* command user issued */
const	char *argnames;			/* helpstring: expected arguments */
struct	scsi_addr dvaddr;		/* SCSI device's address */

extern const char *__progname;		/* from crt0.o */

void	device_format __P((int, char *[]));
void	device_identify __P((int, char *[]));
void	device_reassign __P((int, char *[]));
void	device_reset __P((int, char *[]));

struct command device_commands[] = {
	{ "format",	"",			device_format },
	{ "identify",	"",			device_identify },
	{ "reassign",	"blkno [blkno [...]]",	device_reassign },
	{ "reset",	"",			device_reset },
	{ NULL,		NULL,			NULL },
};

void	bus_reset __P((int, char *[]));
void	bus_scan __P((int, char *[]));

struct command bus_commands[] = {
	{ "reset",	"",			bus_reset },
	{ "scan",	"target lun",		bus_scan },
	{ NULL,		NULL,				NULL },
};

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct command *commands;
	int i;

	/* Must have at least: device command */
	if (argc < 3)
		usage();

	/* Skip program name, get and skip device name and command. */
	dvname = argv[1];
	cmdname = argv[2];
	argv += 3;
	argc -= 3;

	/*
	 * Open the device and determine if it's a scsibus or an actual
	 * device.  Devices respond to the SCIOCIDENTIFY ioctl.
	 */
	fd = opendisk(dvname, O_RDWR, dvname_store, sizeof(dvname_store), 0);
	if (fd == -1) {
		if (errno == ENOENT) {
			/*
			 * Device doesn't exist.  Probably trying to open
			 * a device which doesn't use disk semantics for
			 * device name.  Try again, specifying "cooked",
			 * which leaves off the "r" in front of the device's
			 * name.
			 */
			fd = opendisk(dvname, O_RDWR, dvname_store,
			    sizeof(dvname_store), 1);
			if (fd == -1)
				err(1, "%s", dvname);
		} else
			err(1, "%s", dvname);
	}

	/*
	 * Point the dvname at the actual device name that opendisk() opened.
	 */
	dvname = dvname_store;

	if (ioctl(fd, SCIOCIDENTIFY, &dvaddr) < 0)
		commands = bus_commands;
	else
		commands = device_commands;

	/* Look up and call the command. */
	for (i = 0; commands[i].cmd_name != NULL; i++)
		if (strcmp(cmdname, commands[i].cmd_name) == 0)
			break;
	if (commands[i].cmd_name == NULL)
		errx(1, "unknown %s command: %s\n",
		    commands == bus_commands ? "bus" : "device", cmdname);

	argnames = commands[i].arg_names;

	(*commands[i].cmd_func)(argc, argv);
	exit(0);
}

void
usage()
{
	int i;

	fprintf(stderr, "Usage: %s device command [arg [...]]\n",
	    __progname);

	fprintf(stderr, "   Commands pertaining to scsi devices:\n");
	for (i=0; device_commands[i].cmd_name != NULL; i++)
		fprintf(stderr, "\t%s %s\n", device_commands[i].cmd_name,
					    device_commands[i].arg_names);
	fprintf(stderr, "   Commands pertaining to scsi busses:\n");
	for (i=0; bus_commands[i].cmd_name != NULL; i++)
		fprintf(stderr, "\t%s %s\n", bus_commands[i].cmd_name,
					    bus_commands[i].arg_names);
	fprintf(stderr, "   Use `any' or `all' to wildcard target or lun\n");
	
	exit(1);
}

/*
 * DEVICE COMMANDS
 */

/*
 * device_format:
 *
 *	Format a direct access device.
 *
 *	XXX Does not handle defect list management or geometry settings.
 */
void
device_format(argc, argv)
	int argc;
	char *argv[];
{
	struct scsi_format_unit cmd;
	struct {
		struct scsi_mode_header header;
		struct scsi_blk_desc blk_desc;
		struct page_disk_format format_page;
	} data;

	/* No arguments. */
	if (argc != 0)
		usage();

	/*
	 * Get the DISK FORMAT mode page.  SCSI-2 recommends specifying the
	 * interleave read from this page in the FORMAT UNIT command.
	 */
	scsi_mode_sense(fd, 0x03, 0x00, &data, sizeof(data));

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_FORMAT_UNIT;
	memcpy(cmd.interleave, data.format_page.interleave,
	    sizeof(cmd.interleave));

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 21600000, 0);

	return;
}

/*
 * device_identify:
 *
 *	Display the identity of the device, including it's SCSI bus,
 *	target, lun, and it's vendor/product/revision information.
 */
void
device_identify(argc, argv)
	int argc;
	char *argv[];
{
	struct scsipi_inquiry_data inqbuf;
	struct scsipi_inquiry cmd;

	/* x4 in case every character is escaped, +1 for NUL. */
	char vendor[(sizeof(inqbuf.vendor) * 4) + 1],
	     product[(sizeof(inqbuf.product) * 4) + 1],
	     revision[(sizeof(inqbuf.revision) * 4) + 1];

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));
	memset(&inqbuf, 0, sizeof(inqbuf));

	cmd.opcode = INQUIRY;
	cmd.length = sizeof(inqbuf);

	scsi_command(fd, &cmd, sizeof(cmd), &inqbuf, sizeof(inqbuf),
	    10000, SCCMD_READ);

	scsi_strvis(vendor, sizeof(vendor), inqbuf.vendor,
	    sizeof(inqbuf.vendor));
	scsi_strvis(product, sizeof(product), inqbuf.product,
	    sizeof(inqbuf.product));
	scsi_strvis(revision, sizeof(revision), inqbuf.revision,
	    sizeof(inqbuf.revision));

	printf("%s: scsibus%d target %d lun %d <%s, %s, %s>\n",
	    dvname, dvaddr.addr.scsi.scbus, dvaddr.addr.scsi.target,
	    dvaddr.addr.scsi.lun, vendor, product, revision);

	return;
}

/*
 * device_reassign:
 *
 *	Reassign bad blocks on a direct access device.
 */
void
device_reassign(argc, argv)
	int argc;
	char *argv[];
{
	struct scsi_reassign_blocks cmd;
	struct scsi_reassign_blocks_data *data;
	size_t dlen;
	u_int32_t blkno;
	int i;
	char *cp;

	/* We get a list of block numbers. */
	if (argc < 1)
		usage();

	/*
	 * Allocate the reassign blocks descriptor.  The 4 comes from the
	 * size of the block address in the defect descriptor.
	 */
	dlen = sizeof(struct scsi_reassign_blocks_data) + ((argc - 1) * 4);
	data = malloc(dlen);
	if (data == NULL)
		errx(1, "unable to allocate defect descriptor");
	memset(data, 0, dlen);

	cmd.opcode = SCSI_REASSIGN_BLOCKS;
	cmd.byte2 = 0;
	cmd.unused[0] = 0;
	cmd.unused[1] = 0;
	cmd.unused[2] = 0;
	cmd.control = 0;

	/* Defect descriptor length. */
	_lto2b(argc * 4, data->length);

	/* Build the defect descriptor list. */
	for (i = 0; i < argc; i++) {
		blkno = strtoul(argv[i], &cp, 10);
		if (*cp != '\0')
			errx(1, "invalid block number: %s\n", argv[i]);
		_lto4b(blkno, data->defect_descriptor[i].dlbaddr);
	}

	scsi_command(fd, &cmd, sizeof(cmd), data, dlen, 30000, SCCMD_WRITE);

	free(data);
	return;
}

/*
 * device_reset:
 *
 *	Issue a reset to a SCSI device.
 */
void
device_reset(argc, argv)
	int argc;
	char *argv[];
{

	/* No arguments. */
	if (argc != 0)
		usage();

	if (ioctl(fd, SCIOCRESET, NULL) != 0)
		err(1, "SCIOCRESET");

	return;
}

/*
 * BUS COMMANDS
 */

/*
 * bus_reset:
 *
 *	Issue a reset to a SCSI bus.
 */
void
bus_reset(argc, argv)
	int argc;
	char *argv[];
{

	/* No arguments. */
	if (argc != 0)
		usage();

	if (ioctl(fd, SCBUSIORESET, NULL) != 0)
		err(1, "SCBUSIORESET");

	return;
}

/*
 * bus_scan:
 *
 *	Rescan a SCSI bus for new devices.
 */
void
bus_scan(argc, argv)
	int argc;
	char *argv[];
{
	struct scbusioscan_args args;
	char *cp;

	/* Must have two args: target lun */
	if (argc != 2)
		usage();

	if (strcmp(argv[0], "any") == 0 || strcmp(argv[0], "all") == 0)
		args.sa_target = -1;
	else {
		args.sa_target = strtol(argv[0], &cp, 10);
		if (*cp != '\0' || args.sa_target < 0)
			errx(1, "invalid target: %s\n", argv[0]);
	}

	if (strcmp(argv[1], "any") == 0 || strcmp(argv[1], "all") == 0)
		args.sa_lun = -1;
	else {
		args.sa_lun = strtol(argv[1], &cp, 10);
		if (*cp != '\0' || args.sa_lun < 0)
			errx(1, "invalid lun: %s\n", argv[1]);
	}

	if (ioctl(fd, SCBUSIOSCAN, &args) != 0)
		err(1, "SCBUSIOSCAN");

	return;
}
