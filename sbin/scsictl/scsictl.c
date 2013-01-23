/*	$NetBSD: scsictl.c,v 1.33.2.2 2013/01/23 00:05:34 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 2002 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: scsictl.c,v 1.33.2.2 2013/01/23 00:05:34 yamt Exp $");
#endif


#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/scsipi/scsi_spc.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsipiconf.h>

#include "extern.h"

struct command {
	const char *cmd_name;
	const char *arg_names;
	void (*cmd_func)(int, char *[]);
};

__dead static void	usage(void);

static int	fd;				/* file descriptor for device */
const  char	*dvname;			/* device name */
static char	dvname_store[MAXPATHLEN];	/* for opendisk(3) */
static const	char *cmdname;			/* command user issued */
static struct	scsi_addr dvaddr;		/* SCSI device's address */

static void	device_defects(int, char *[]);
static void	device_format(int, char *[]);
static void	device_identify(int, char *[]);
static void	device_reassign(int, char *[]);
static void	device_release(int, char *[]);
static void	device_reserve(int, char *[]);
static void	device_reset(int, char *[]);
static void	device_debug(int, char *[]);
static void	device_prevent(int, char *[]);
static void	device_allow(int, char *[]);
static void	device_start(int, char *[]);
static void	device_stop(int, char *[]);
static void	device_tur(int, char *[]);
static void	device_getcache(int, char *[]);
static void	device_setcache(int, char *[]);
static void	device_flushcache(int, char *[]);
static void	device_setspeed(int, char *[]);

static struct command device_commands[] = {
	{ "defects",	"[primary] [grown] [block|byte|physical]",
						device_defects },
	{ "format",	"[blocksize [immediate]]", 	device_format },
	{ "identify",	"",			device_identify },
	{ "reassign",	"blkno [blkno [...]]",	device_reassign },
	{ "release",	"",			device_release },
	{ "reserve",	"",			device_reserve },
	{ "reset",	"",			device_reset },
	{ "debug",	"level",		device_debug },
	{ "prevent",	"",			device_prevent },
	{ "allow",	"",			device_allow },
	{ "start",	"",			device_start },
	{ "stop",	"",			device_stop },
	{ "tur",	"",			device_tur },
	{ "getcache",	"",			device_getcache },
	{ "setcache",	"none|r|w|rw [save]",	device_setcache },
	{ "flushcache",	"",			device_flushcache },
	{ "setspeed",	"[speed]",		device_setspeed },
	{ NULL,		NULL,			NULL },
};

static void	bus_reset(int, char *[]);
static void	bus_scan(int, char *[]);
static void	bus_detach(int, char *[]);

static struct command bus_commands[] = {
	{ "reset",	"",			bus_reset },
	{ "scan",	"target lun",		bus_scan },
	{ "detach",	"target lun",		bus_detach },
	{ NULL,		NULL,				NULL },
};

int
main(int argc, char *argv[])
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

	(*commands[i].cmd_func)(argc, argv);
	exit(0);
}

static void
usage(void)
{
	int i;

	fprintf(stderr, "usage: %s device command [arg [...]]\n",
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
 * device_read_defect:
 *
 *	Read primary and/or growth defect list in physical or block
 *	format from a direct access device.
 *
 *	XXX Does not handle very large defect lists. Needs SCSI3 12
 *	    byte READ DEFECT DATA command.
 */

static void	print_bf_dd(union scsi_defect_descriptor *);
static void	print_bfif_dd(union scsi_defect_descriptor *);
static void	print_psf_dd(union scsi_defect_descriptor *);

static void
device_defects(int argc, char *argv[])
{
	struct scsi_read_defect_data cmd;
	struct scsi_read_defect_data_data *data;
	size_t dlen;
	int i, dlfmt = -1;
	int defects;
	char msg[256];
	void (*pfunc)(union scsi_defect_descriptor *);
#define RDD_P_G_MASK	0x18
#define RDD_DLF_MASK	0x7

	dlen = USHRT_MAX; 		/* XXX - this may not be enough room
					 * for all of the defects.
					 */
	data = malloc(dlen);
	if (data == NULL)
		errx(1, "unable to allocate defect list");
	memset(data, 0, dlen);
	memset(&cmd, 0, sizeof(cmd));
	defects = 0;
	pfunc = NULL;

	/* determine which defect list(s) to read. */
	for (i = 0; i < argc; i++) {
		if (strncmp("primary", argv[i], 7) == 0) {
			cmd.flags |= RDD_PRIMARY;
			continue;
		}
		if (strncmp("grown", argv[i], 5) == 0) {
			cmd.flags |= RDD_GROWN;
			continue;
		}
		break;
	}

	/* no defect list sepecified, assume both. */
	if ((cmd.flags & (RDD_PRIMARY|RDD_GROWN)) == 0)
		cmd.flags |= (RDD_PRIMARY|RDD_GROWN);

	/* list format option. */
	if (i < argc) { 
		if (strncmp("block", argv[i], 5) == 0) {
			cmd.flags |= RDD_BF;
			dlfmt = RDD_BF;
		}
		else if (strncmp("byte", argv[i], 4) == 0) {
			cmd.flags |= RDD_BFIF;
			dlfmt = RDD_BFIF;
		}
		else if (strncmp("physical", argv[i], 4) == 0) {
			cmd.flags |= RDD_PSF;
			dlfmt = RDD_PSF;
		}
		else {
			usage();
		}
	}

	/*
	 * no list format specified; since block format not
	 * recommended use physical sector format as default.
	 */
	if (dlfmt < 0) {
		cmd.flags |= RDD_PSF;
		dlfmt = RDD_PSF;
	}

	cmd.opcode = SCSI_READ_DEFECT_DATA;
	_lto2b(dlen, &cmd.length[0]);

	scsi_command(fd, &cmd, sizeof(cmd), data, dlen, 30000, SCCMD_READ);

	msg[0] = '\0';

	/* is the defect list in the format asked for? */
	if ((data->flags & RDD_DLF_MASK) != dlfmt) {
		strcpy(msg, "\n\tnotice:"
		       "requested defect list format not supported by device\n\n");
		dlfmt = (data->flags & RDD_DLF_MASK);
	}

	if (data->flags & RDD_PRIMARY)
		strcat(msg, "primary");

	if (data->flags & RDD_GROWN) {
		if (data->flags & RDD_PRIMARY)
			strcat(msg, " and ");
		strcat(msg, "grown");
	}

	strcat(msg, " defects");

	if ((data->flags & RDD_P_G_MASK) == 0)
		strcat(msg, ": none reported\n");


	printf("%s: scsibus%d target %d lun %d %s",
	       dvname, dvaddr.addr.scsi.scbus, dvaddr.addr.scsi.target,
	       dvaddr.addr.scsi.lun, msg);

	/* device did not return either defect list. */
	if ((data->flags & RDD_P_G_MASK) == 0)
		return;

	switch (dlfmt) {
	case RDD_BF:
		defects = _2btol(data->length) /
				sizeof(struct scsi_defect_descriptor_bf);
		pfunc = print_bf_dd;
		strcpy(msg, "block address\n"
			    "-------------\n");
		break;
	case RDD_BFIF:
		defects = _2btol(data->length) /
				sizeof(struct scsi_defect_descriptor_bfif);
		pfunc = print_bfif_dd;
		strcpy(msg, "              bytes from\n"
			    "cylinder head   index\n"
			    "-------- ---- ----------\n");
		break;
	case RDD_PSF:
		defects = _2btol(data->length) /
				sizeof(struct scsi_defect_descriptor_psf);
		pfunc = print_psf_dd;
		strcpy(msg, "cylinder head   sector\n"
			    "-------- ---- ----------\n");
		break;
	}

	/* device did not return any defects. */
	if (defects == 0) {
		printf(": none\n");
		return;
	}

	printf(": %d\n", defects);

	/* print heading. */
	printf("%s", msg);

	/* print defect list. */
	for (i = 0 ; i < defects; i++) {
		pfunc(&data->defect_descriptor[i]);
	}

	free(data);
	return;
}

/*
 * print_bf_dd:
 *
 *	Print a block format defect descriptor.
 */
static void
print_bf_dd(union scsi_defect_descriptor *dd)
{
	u_int32_t block;

	block = _4btol(dd->bf.block_address);

	printf("%13u\n", block);
}

#define DEFECTIVE_TRACK	0xffffffff

/*
 * print_bfif_dd:
 *
 *	Print a bytes from index format defect descriptor.
 */
static void
print_bfif_dd(union scsi_defect_descriptor *dd)
{
	u_int32_t cylinder;
	u_int32_t head;
	u_int32_t bytes_from_index;

	cylinder = _3btol(dd->bfif.cylinder);
	head = dd->bfif.head;
	bytes_from_index = _4btol(dd->bfif.bytes_from_index);

	printf("%8u %4u ", cylinder, head);

	if (bytes_from_index == DEFECTIVE_TRACK)
		printf("entire track defective\n");
	else
		printf("%10u\n", bytes_from_index);
}

/*
 * print_psf_dd:
 *
 *	Print a physical sector format defect descriptor.
 */
static void
print_psf_dd(union scsi_defect_descriptor *dd)
{
	u_int32_t cylinder;
	u_int32_t head;
	u_int32_t sector;

	cylinder = _3btol(dd->psf.cylinder);
	head = dd->psf.head;
	sector = _4btol(dd->psf.sector);

	printf("%8u %4u ", cylinder, head);

	if (sector == DEFECTIVE_TRACK)
		printf("entire track defective\n");
	else
		printf("%10u\n", sector);
}

/*
 * device_format:
 *
 *	Format a direct access device.
 */
static void
device_format(int argc, char *argv[])
{
	u_int32_t blksize;
	int i, j, immediate;
#define	PC	(65536/10)
	static int complete[] = {
	    PC*1, PC*2, PC*3, PC*4, PC*5, PC*6, PC*7, PC*8, PC*9, 65536
	};
	char *cp, buffer[64];
	struct scsi_sense_data sense;
	struct scsi_format_unit cmd;
	struct {
		struct scsi_format_unit_defect_list_header header;
		/* optional initialization pattern */
		/* optional defect list */
	} dfl;
	struct {
		struct scsi_mode_parameter_header_6 header;
		struct scsi_general_block_descriptor blk_desc;
		struct page_disk_format format_page;
	} mode_page;
	struct {
		struct scsi_mode_parameter_header_6 header;
		struct scsi_general_block_descriptor blk_desc;
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
		if ((j = SSD_SENSE_KEY(sense.flags)) == SKEY_NO_SENSE)
			break;
	}
	/*
	 * Make sure we cleared any pending Unit Attention
	 */
	if (j != SKEY_NO_SENSE) {
		cp = scsi_decode_sense((const unsigned char *) &sense, 2,
		    buffer, sizeof (buffer));
		errx(1, "failed to clean Unit Attention: %s", cp);
	}

	/*
	 * Get the DISK FORMAT mode page.  SCSI-2 recommends specifying the
	 * interleave read from this page in the FORMAT UNIT command.
	 */
	scsi_mode_sense(fd, 0x03, 0x00, &mode_page, sizeof(mode_page));

	j = (mode_page.format_page.bytes_s[0] << 8) |
	    (mode_page.format_page.bytes_s[1]);

	if (j != DEV_BSIZE)
		printf("current disk sector size: %d\n", j);

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
			errx(1, "invalid block size: %s", argv[0]);

		memset(&data_select, 0, sizeof(data_select));

		data_select.header.blk_desc_len =
		    sizeof(struct scsi_general_block_descriptor);
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
	    8 * 60 * 60 * 1000, SCCMD_WRITE);

	/*
	 * Poll device for completion of Format
	 */
	if (immediate) {
		i = 0;
		printf("formatting.");
		fflush(stdout);
		do {
			scsireq_t req;
			struct scsi_test_unit_ready tcmd;

			memset(&tcmd, 0, sizeof(tcmd));
			tcmd.opcode = SCSI_TEST_UNIT_READY;

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
			memcpy(&sense, req.sense, sizeof(sense));
			if (sense.sks.sks_bytes[0] & SSD_SKSV) {
				j = (sense.sks.sks_bytes[1] << 8) |
				    (sense.sks.sks_bytes[2]);
				if (j >= complete[i]) {
					printf(".%d0%%.", ++i);
					fflush(stdout);
				}
			}
			sleep(10);
		} while (SSD_SENSE_KEY(sense.flags) == SKEY_NOT_READY);
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
static void
device_identify(int argc, char *argv[])
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
static void
device_reassign(int argc, char *argv[])
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
 *	Issue a RELEASE command to a SCSI device.
 */
#ifndef	SCSI_RELEASE
#define	SCSI_RELEASE	0x17
#endif
static void
device_release(int argc, char *argv[])
{
	struct scsi_test_unit_ready cmd;	/* close enough */

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
 *	Issue a RESERVE command to a SCSI device.
 */
#ifndef	SCSI_RESERVE
#define	SCSI_RESERVE	0x16
#endif
static void
device_reserve(int argc, char *argv[])
{
	struct scsi_test_unit_ready cmd;	/* close enough */

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
static void
device_reset(int argc, char *argv[])
{

	/* No arguments. */
	if (argc != 0)
		usage();

	if (ioctl(fd, SCIOCRESET, NULL) != 0)
		err(1, "SCIOCRESET");

	return;
}

/*
 * device_debug:
 *
 *	Set debug level to a SCSI device.
 *	scsipi will print anything iff SCSIPI_DEBUG set in config.
 */
static void
device_debug(int argc, char *argv[])
{
	int lvl;

	if (argc < 1)
		usage();

	lvl = atoi(argv[0]);

	if (ioctl(fd, SCIOCDEBUG, &lvl) != 0)
		err(1, "SCIOCDEBUG");

	return;
}

/*
 * device_getcache:
 *
 *	Get the caching parameters for a SCSI disk.
 */
static void
device_getcache(int argc, char *argv[])
{
	struct {
		struct scsi_mode_parameter_header_6 header;
		struct scsi_general_block_descriptor blk_desc;
		struct page_caching caching_params;
	} data;

	/* No arguments. */
	if (argc != 0)
		usage();

	scsi_mode_sense(fd, 0x08, 0x00, &data, sizeof(data));

	if ((data.caching_params.flags & (CACHING_RCD|CACHING_WCE)) ==
	    CACHING_RCD)
		printf("%s: no caches enabled\n", dvname);
	else {
		printf("%s: read cache %senabled\n", dvname,
		    (data.caching_params.flags & CACHING_RCD) ? "not " : "");
		printf("%s: write-back cache %senabled\n", dvname,
		    (data.caching_params.flags & CACHING_WCE) ? "" : "not ");
	}
	printf("%s: caching parameters are %ssavable\n", dvname,
	    (data.caching_params.pg_code & PGCODE_PS) ? "" : "not ");
}

/*
 * device_setcache:
 *
 *	Set cache enables for a SCSI disk.
 */
static void
device_setcache(int argc, char *argv[])
{
	struct {
		struct scsi_mode_parameter_header_6 header;
		struct scsi_general_block_descriptor blk_desc;
		struct page_caching caching_params;
	} data;
	int dlen;
	u_int8_t flags, byte2;

	if (argc > 2 || argc == 0)
		usage();

	flags = 0;
	byte2 = 0;
	if (strcmp(argv[0], "none") == 0)
		flags = CACHING_RCD;
	else if (strcmp(argv[0], "r") == 0)
		flags = 0;
	else if (strcmp(argv[0], "w") == 0)
		flags = CACHING_RCD|CACHING_WCE;
	else if (strcmp(argv[0], "rw") == 0)
		flags = CACHING_WCE;
	else
		usage();

	if (argc == 2) {
		if (strcmp(argv[1], "save") == 0)
			byte2 = SMS_SP;
		else
			usage();
	}

	scsi_mode_sense(fd, 0x08, 0x00, &data, sizeof(data));

	data.caching_params.pg_code &= PGCODE_MASK;
	data.caching_params.flags =
	    (data.caching_params.flags & ~(CACHING_RCD|CACHING_WCE)) | flags;

	data.caching_params.cache_segment_size[0] = 0;
	data.caching_params.cache_segment_size[1] = 0;

	data.header.data_length = 0;

	dlen = sizeof(data.header) + sizeof(data.blk_desc) + 2 +
	    data.caching_params.pg_length;

	scsi_mode_select(fd, byte2, &data, dlen);
}

/*
 * device_flushcache:
 *
 *	Issue a FLUSH CACHE command to a SCSI device.
 */
#ifndef	SCSI_FLUSHCACHE
#define	SCSI_FLUSHCACHE	0x35
#endif
static void
device_flushcache(int argc, char *argv[])
{
	struct scsi_test_unit_ready cmd;	/* close enough */

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_FLUSHCACHE;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

	return;
}

/*
 * device_setspeed:
 *
 *	Set rotation speed to a CD/DVD drive.
 */
static void
device_setspeed(int argc, char *argv[])
{
	u_char cmd[11];
	u_char pd[28];
	u_int32_t speed;

	if (argc != 1)
		usage();

	speed = atoi(argv[0]) * 177;

	memset(&pd, 0, sizeof(pd));
	if (speed == 0)
		pd[0] = 4; /* restore drive defaults */
	pd[8] = 0xff;
	pd[9] = 0xff;
	pd[10] = 0xff;
	pd[11] = 0xff;
	pd[12] = pd[20] = (speed >> 24) & 0xff;
	pd[13] = pd[21] = (speed >> 16) & 0xff;
	pd[14] = pd[22] = (speed >> 8) & 0xff;
	pd[15] = pd[23] = speed & 0xff;
	pd[18] = pd[26] = 1000 >> 8;
	pd[19] = pd[27] = 1000 & 0xff;

	memset(&cmd, 0, sizeof(cmd));
	cmd[0] = 0xb6;
	cmd[10] = sizeof(pd);

	scsi_command(fd, &cmd, sizeof(cmd), pd, sizeof(pd), 10000, SCCMD_WRITE);

	return;
}

/*
 * device_prevent:
 *
 *      Issue a prevent to a SCSI device.
 */
static void
device_prevent(int argc, char *argv[])
{
	struct scsi_prevent_allow_medium_removal cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL;
	cmd.how = SPAMR_PREVENT_DT;	/* XXX SMAMR_PREVENT_ALL? */

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

	return;
}

/*
 * device_allow:
 *
 *      Issue a stop to a SCSI device.
 */
static void
device_allow(int argc, char *argv[])
{
	struct scsi_prevent_allow_medium_removal cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL;
	cmd.how = SPAMR_ALLOW;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

	return;
}

/*
 * device_start:
 *
 *      Issue a start to a SCSI device.
 */
static void
device_start(int argc, char *argv[])
{
	struct scsipi_start_stop cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = START_STOP;
	cmd.how = SSS_START;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 30000, 0);

	return;
}

/*
 * device_stop:
 *
 *      Issue a stop to a SCSI device.
 */
static void
device_stop(int argc, char *argv[])
{
	struct scsipi_start_stop cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = START_STOP;
	cmd.how = SSS_STOP;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 30000, 0);

	return;
}

/*
 * device_tur:
 *
 *	Issue a TEST UNIT READY to a SCSI device.
 */
static void
device_tur(int argc, char *argv[])
{
	struct scsi_test_unit_ready cmd;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_TEST_UNIT_READY;

	scsi_command(fd, &cmd, sizeof(cmd), NULL, 0, 10000, 0);

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
static void
bus_reset(int argc, char *argv[])
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
static void
bus_scan(int argc, char *argv[])
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
static void
bus_detach(int argc, char *argv[])
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
			errx(1, "invalid target: %s", argv[0]);
	}

	if (strcmp(argv[1], "any") == 0 || strcmp(argv[1], "all") == 0)
		args.sa_lun = -1;
	else {
		args.sa_lun = strtol(argv[1], &cp, 10);
		if (*cp != '\0' || args.sa_lun < 0)
			errx(1, "invalid lun: %s", argv[1]);
	}

	if (ioctl(fd, SCBUSIODETACH, &args) != 0)
		err(1, "SCBUSIODETACH");

	return;
}
