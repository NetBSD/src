/*	$NetBSD: scsictl.c,v 1.16 2002/06/26 16:04:14 mjacob Exp $	*/

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

void	device_format __P((int, char *[]));
void	device_identify __P((int, char *[]));
void	device_reassign __P((int, char *[]));
void	device_release __P((int, char *[]));
void	device_reserve __P((int, char *[]));
void	device_reset __P((int, char *[]));
void	device_start __P((int, char *[]));
void	device_stop __P((int, char *[]));
void	device_tur __P((int, char *[]));

struct command device_commands[] = {
	{ "format",	"[blocksize [immediate]]", 	device_format },
	{ "identify",	"",			device_identify },
	{ "reassign",	"blkno [blkno [...]]",	device_reassign },
	{ "release",	"",			device_release },
	{ "reserve",	"",			device_reserve },
	{ "reset",	"",			device_reset },
	{ "start",	"",			device_start },
	{ "stop",	"",			device_stop },
	{ "tur",	"",			device_tur },
	{ NULL,		NULL,			NULL },
};

void	bus_reset __P((int, char *[]));
void	bus_scan __P((int, char *[]));
void	bus_detach __P((int, char *[]));

struct command bus_commands[] = {
	{ "reset",	"",			bus_reset },
	{ "scan",	"target lun",		bus_scan },
	{ "detach",	"target lun",		bus_detach },
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
		errx(1, "unknown %s command: %s",
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
	    getprogname());

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
 */
void
device_format(argc, argv)
	int argc;
	char *argv[];
{
	u_int32_t blksize;
	int i, j, immediate;
#define	PC	(65536/10)
	static int complete[] = {
	    PC*1, PC*2, PC*3, PC*4, PC*5, PC*6, PC*7, PC*8, PC*9, 65536
	};
	char *cp, buffer[64];
	struct scsipi_sense_data sense;
	struct scsi_format_unit cmd;
	struct {
		struct scsi_format_unit_defect_list_header header;
		/* optional initialization pattern */
		/* optional defect list */
	} dfl;
	struct {
		struct scsipi_mode_header header;
		struct scsi_blk_desc blk_desc;
		struct page_disk_format format_page;
	} mode_page;
	struct {
		struct scsipi_mode_header header;
		struct scsi_blk_desc blk_desc;
	} data_select;
	

	/* Blocksize is an optional argument. */
	if (argc > 2)
		usage();

	/*
	 * Loop doing Request Sense to clear any pending Unit Attention.
	 *
	 * Multiple conditions may exist on the drive which are returned
	 * in priority order.
	 */
	for (i = 0; i < 8; i++) {
		scsi_request_sense(fd, &sense, sizeof (sense));
		if ((j = sense.flags & SSD_KEY) == SKEY_NO_SENSE)
			break;
	}
	/*
	 * Make sure we cleared any pending Unit Attention
	 */
	if (j != SKEY_NO_SENSE) {
		cp = scsi_decode_sense((const unsigned char *) &sense, 2,
		    buffer, sizeof (buffer));
		errx(1, "failed to clean Unit Attention: %s\n", cp);
	}

	/*
	 * Get the DISK FORMAT mode page.  SCSI-2 recommends specifying the
	 * interleave read from this page in the FORMAT UNIT command.
	 */
	scsi_mode_sense(fd, 0x03, 0x00, &mode_page, sizeof(mode_page));

	j = (mode_page.format_page.bytes_s[0] << 8) |
	    (mode_page.format_page.bytes_s[1]);

	if (j != DEV_BSIZE)
		printf("current disk sector size: %hd\n", j);

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_FORMAT_UNIT;
	memcpy(cmd.interleave, mode_page.format_page.interleave,
	    sizeof(cmd.interleave));

	/*
	 * The blocksize on the device is only changed if the user
	 * specified a new blocksize. If not specified the blocksize
	 * used for the device will be the Default value in the device.
	 * We don't specify the number of blocks since the format
	 * command will always reformat the entire drive.  Also by
	 * not specifying a block count the drive will reset the
	 * block count to the maximum available after the format
	 * completes if the blocksize was changed in the format.
	 * Finally, the new disk geometry will not but updated on
	 * the drive in permanent storage until _AFTER_ the format
	 * completes successfully.
	 */
	if (argc > 0) {
		blksize = strtoul(argv[0], &cp, 10);
		if (*cp != '\0')
			errx(1, "invalid block size: %s\n", argv[0]);

		memset(&data_select, 0, sizeof(data_select));

		data_select.header.blk_desc_len = sizeof(struct scsi_blk_desc);
		/*
		 * blklen in desc is 3 bytes with a leading reserved byte
		 */
		_lto4b(blksize, &data_select.blk_desc.reserved);

		/*
		 * Issue Mode Select to modify the device blocksize to be
		 * used on the Format.  The modified device geometry will
		 * be stored as Current and Saved Page 3 parameters when
		 * the Format completes.
		 */
		scsi_mode_select(fd, 0, &data_select, sizeof(data_select));

		/*
		 * Since user specified a specific block size make sure it
		 * gets stored in the device when the format completes.
		 *
		 * Also scrub the defect list back to the manufacturers
		 * original.
		 */
		cmd.flags = SFU_CMPLST | SFU_FMTDATA;
	}

	memset(&dfl, 0, sizeof(dfl));

	if (argc > 1 && strncmp(argv[1], "imm", 3) == 0) {
		/*
		 * Signal target for an immediate return from Format.
		 *
		 * We'll poll for completion status.
		 */
		dfl.header.flags = DLH_IMMED;
		immediate = 1;
	} else {
		immediate = 0;
	}

	scsi_command(fd, &cmd, sizeof(cmd), &dfl, sizeof(dfl),
	    8 * 60 * 60 * 1000, 0);

	/*
	 * Poll device for completion of Format
	 */
	if (immediate) {
		i = 0;
		printf("formatting.");
		fflush(stdout);
		do {
			scsireq_t req;
			struct scsipi_test_unit_ready tcmd;

			memset(&tcmd, 0, sizeof(cmd));
			tcmd.opcode = TEST_UNIT_READY;

			memset(&req, 0, sizeof(req));
			memcpy(req.cmd, &tcmd, 6);
			req.cmdlen = 6;
			req.timeout = 10000;
			req.senselen = SENSEBUFLEN;

			if (ioctl(fd, SCIOCCOMMAND, &req) == -1) {
				err(1, "SCIOCCOMMAND");
			}

			if (req.retsts == SCCMD_OK) {
				break;
			} else if (req.retsts == SCCMD_TIMEOUT) {
				fprintf(stderr, "%s: SCSI command timed out",
				    dvname);
				break;
			} else if (req.retsts == SCCMD_BUSY) {
				fprintf(stderr, "%s: device is busy",
				    dvname);
				break;
			} else if (req.retsts != SCCMD_SENSE) {
				fprintf(stderr,
				    "%s: device had unknown status %x", dvname,
				    req.retsts);
				break;
			}
			memcpy(&sense, req.sense, SENSEBUFLEN);
			if (sense.sense_key_spec_1 == SSD_SCS_VALID) {
				j = (sense.sense_key_spec_2 << 8) |
				    (sense.sense_key_spec_3);
				if (j >= complete[i]) {
					printf(".%d0%%.", ++i);
					fflush(stdout);
				}
			}
			sleep(10);
		} while ((sense.flags & SSD_KEY) == SKEY_NOT_READY);
		printf(".100%%..done.\n");
	}
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
			errx(1, "invalid block number: %s", argv[i]);
		_lto4b(blkno, data->defect_descriptor[i].dlbaddr);
	}

	scsi_command(fd, &cmd, sizeof(cmd), data, dlen, 30000, SCCMD_WRITE);

	free(data);
	return;
}

/*
 * device_release:
 *
 *      Issue a RELEASE command to a SCSI drevice
 */
#ifndef	SCSI_RELEASE
#define	SCSI_RELEASE	0x17
#endif
void
device_release(argc, argv)
	int argc;
	char *argv[];
{
	struct scsipi_test_unit_ready cmd;	/* close enough */

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_RELEASE;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

	return;
}



/*
 * device_reserve:
 *
 *      Issue a RESERVE command to a SCSI drevice
 */
#ifndef	SCSI_RESERVE
#define	SCSI_RESERVE	0x16
#endif
void
device_reserve(argc, argv)
	int argc;
	char *argv[];
{
	struct scsipi_test_unit_ready cmd;	/* close enough */

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_RESERVE;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

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
 * device_start:
 *
 *      Issue a start to a SCSI device.
 */
void
device_start(argc, argv)
	int argc;
	char *argv[];
{
	struct scsipi_start_stop cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = START_STOP;
	cmd.how = SSS_START;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

	return;
}

/*
 * device_stop:
 *
 *      Issue a stop to a SCSI device.
 */
void
device_stop(argc, argv)
	int argc;
	char *argv[];
{
	struct scsipi_start_stop cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = START_STOP;
	cmd.how = SSS_STOP;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

	return;
}

/*
 * device_tur:
 *
 *      Issue a TEST UNIT READY to a SCSI drevice
 */
void
device_tur(argc, argv)
	int argc;
	char *argv[];
{
	struct scsipi_test_unit_ready cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = TEST_UNIT_READY;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

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
			errx(1, "invalid target: %s", argv[0]);
	}

	if (strcmp(argv[1], "any") == 0 || strcmp(argv[1], "all") == 0)
		args.sa_lun = -1;
	else {
		args.sa_lun = strtol(argv[1], &cp, 10);
		if (*cp != '\0' || args.sa_lun < 0)
			errx(1, "invalid lun: %s", argv[1]);
	}

	if (ioctl(fd, SCBUSIOSCAN, &args) != 0)
		err(1, "SCBUSIOSCAN");

	return;
}

/*
 * bus_detach:
 *
 *	detach SCSI devices from a bus.
 */
void
bus_detach(argc, argv)
	int argc;
	char *argv[];
{
	struct scbusiodetach_args args;
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

	if (ioctl(fd, SCBUSIODETACH, &args) != 0)
		err(1, "SCBUSIODETACH");

	return;
}
