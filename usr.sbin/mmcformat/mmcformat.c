/* $NetBSD: mmcformat.c,v 1.3.14.1 2014/08/20 00:05:10 tls Exp $ */

/*
 * Copyright (c) 2006, 2008 Reinoud Zandijk
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>
#include <inttypes.h>

#include "uscsilib.h"


/* globals */
struct uscsi_dev dev;
extern int scsilib_verbose;

/* #define DEBUG(a) {a;} */
#define DEBUG(a) ;


static uint64_t
getmtime(void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);
	return (uint64_t) 1000000 * tp.tv_sec + tp.tv_usec;
}


static void
print_eta(uint32_t progress, uint64_t now, uint64_t start_time)
{
	int hours, minutes, seconds;
	uint64_t tbusy, ttot_est, eta;

	if (progress == 0) {
		printf(" ETA --:--:--");
		return;
	}
	tbusy    = now - start_time;
	ttot_est = (tbusy * 0x10000) / progress;
	eta      = (ttot_est - tbusy) / 1000000;

	hours   = (int) (eta/3600);
	minutes = (int) (eta/60) % 60;
	seconds = (int)  eta % 60;
	printf(" ETA %02d:%02d:%02d", hours, minutes, seconds);
}


static void
uscsi_waitop(struct uscsi_dev *mydev)
{
	scsicmd cmd;
	struct uscsi_sense sense;
	uint64_t start_time;
	uint32_t progress;
	uint8_t buffer[256];
	int asc, ascq;
	int cnt = 0;

	bzero(cmd, SCSI_CMD_LEN);
	bzero(buffer, sizeof(buffer));

	/*
	 * not be to unpatient... give the drive some time to start or it
	 * might break off
	 */

	start_time = getmtime();
	sleep(10);

	progress = 0;
	while (progress < 0x10000) {
		/* we need a command that is NOT going to stop the formatting */
		bzero(cmd, SCSI_CMD_LEN);
		cmd[0] = 0;			/* test unit ready */
		uscsi_command(SCSI_READCMD, mydev,
			cmd, 6, buffer, 0, 10000, &sense);

		/*
		 * asc may be `not-ready' or `no-sense'. ascq for format in
		 * progress is 4 too
		 */
		asc  = sense.asc;
		ascq = sense.ascq;
		if (((asc == 0) && (ascq == 4)) || (asc == 4)) {
			/* drive not ready : operation/format in progress */
			if (sense.skey_valid) {
				progress = sense.sense_key;
			} else {
				/* finished */
				progress = 0x10000;
			}
		}
		/* check if drive is ready again, ifso break out loop */
		if ((asc == 0) && (ascq == 0)) {
			progress = 0x10000;
		}

		printf("%3d %% ", (100 * progress / 0x10000));
		printf("%c", "|/-\\" [cnt++ %4]);   /* twirl */

		/* print ETA */
		print_eta(progress, getmtime(), start_time);

		fflush(stdout);
		sleep(1);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	}
	printf("\n");

	return;
}


static char const *
print_mmc_profile(int profile)
{
	static char scrap[100];

	switch (profile) {
	case 0x00 : return "Unknown[0] profile";
	case 0x01 : return "Non removeable disc";
	case 0x02 : return "Removable disc";
	case 0x03 : return "Magneto Optical with sector erase";
	case 0x04 : return "Magneto Optical write once";
	case 0x05 : return "Advance Storage Magneto Optical";
	case 0x08 : return "CD-ROM";
	case 0x09 : return "CD-R recordable";
	case 0x0a : return "CD-RW rewritable";
	case 0x10 : return "DVD-ROM";
	case 0x11 : return "DVD-R sequential";
	case 0x12 : return "DVD-RAM rewritable";
	case 0x13 : return "DVD-RW restricted overwrite";
	case 0x14 : return "DVD-RW sequential";
	case 0x1a : return "DVD+RW rewritable";
	case 0x1b : return "DVD+R recordable";
	case 0x20 : return "DDCD readonly";
	case 0x21 : return "DDCD-R recordable";
	case 0x22 : return "DDCD-RW rewritable";
	case 0x2b : return "DVD+R double layer";
	case 0x40 : return "BD-ROM";
	case 0x41 : return "BD-R Sequential Recording (SRM)";
	case 0x42 : return "BD-R Random Recording (RRM)";
	case 0x43 : return "BD-RE rewritable";
	}
	sprintf(scrap, "Reserved profile 0x%02x", profile);
	return scrap;
}


static int
uscsi_get_mmc_profile(struct uscsi_dev *mydev, int *mmc_profile)
{
	scsicmd	 cmd;
	uint8_t  buf[32];
	int error;

	*mmc_profile = 0;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x46;				/* Get configuration */
	cmd[ 8] = 32;				/* just a small buffer size */
	cmd[ 9] = 0;				/* control */
	error = uscsi_command(SCSI_READCMD, mydev, cmd, 10, buf, 32, 30000, NULL);
	if (!error) {
		*mmc_profile = buf[7] | (buf[6] << 8);
	}

	return error;
}


static int
uscsi_set_packet_parameters(struct uscsi_dev *mydev, int blockingnr)
{
	scsicmd  cmd;
	int      val_len;
	uint8_t  res[10000], *pos;
	int      error;

	/* Set up CD/DVD recording parameters */
	DEBUG(printf("Setting device's recording parameters\n"));

	val_len = 0x32+2+8;
	bzero(res, val_len);

	pos = res + 8;

	bzero(cmd, SCSI_CMD_LEN);
	pos[ 0] = 0x05;		/* page code 5 : cd writing		*/
	pos[ 1] = 0x32;		/* length in bytes			*/
	pos[ 2] = 0;		/* write type 0 : packet/incremental	*/

	/* next session OK, data packet, rec. incr. fixed packets	*/
	pos[ 3] = (3<<6) | 32 | 5;
	pos[ 4] = 10;		/* ISO mode 2; XA form 1		*/
	pos[ 8] = 0x20;		/* CD-ROM XA disc or DDCD disc		*/
	pos[10] = (blockingnr >> 24) & 0xff;	/* MSB packet size 	*/
	pos[11] = (blockingnr >> 16) & 0xff;
	pos[12] = (blockingnr >>  8) & 0xff;
	pos[13] = (blockingnr      ) & 0xff;	/* LSB packet size 	*/

	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x55;			/* MODE SELECT (10)		*/
	cmd[1] = 16;			/* PF format			*/
	cmd[7] = val_len >> 8;		/* length of blob		*/
	cmd[8] = val_len & 0xff;
	cmd[9] = 0;			/* control			*/

	error = uscsi_command(SCSI_WRITECMD, mydev,
			cmd, 10, res, val_len, 30000, NULL);
	if (error) {
		perror("While WRTITING parameter page 5");
		return error;
	}

	/* flag OK */
	return 0;
}


static int
get_format_capabilities(struct uscsi_dev *mydev, uint8_t *buf, uint32_t *len)
{
	scsicmd		cmd;
	int		list_length;
	int		trans_len;
	size_t		buf_len = 512;
	int		error;

	assert(*len >= buf_len);
	bzero(buf, buf_len);

	trans_len = 12;				/* only fixed header first */
	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x23;				/* Read format capabilities */
	cmd[7] = trans_len >> 8;		/* MSB allocation length */
	cmd[8] = trans_len & 0xff;		/* LSB allocation length */
	cmd[9] = 0;				/* control */
	error = uscsi_command(SCSI_READCMD, mydev,
			cmd, 10, buf, trans_len, 30000, NULL);
	if (error) {
		fprintf(stderr, "While reading format capabilities : %s\n",
			strerror(error));
		return error;
	}

	list_length = buf[ 3];

	if (list_length % 8) {
		printf( "\t\tWarning: violating SCSI spec,"
			"capacity list length ought to be multiple of 8\n");
		printf("\t\tInterpreting as including header of 4 bytes\n");
		assert(list_length % 8 == 4);
		list_length -= 4;
	}

	/* read in full capacity list */
	trans_len = 12 + list_length;		/* complete structure */
	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x23;				/* Read format capabilities */
	cmd[7] = trans_len >> 8;		/* MSB allocation length */
	cmd[8] = trans_len & 0xff;		/* LSB allocation length */
	cmd[9] = 0;				/* control */
	error = uscsi_command(SCSI_READCMD, mydev,
			cmd, 10, buf, trans_len, 30000, NULL);
	if (error) {
		fprintf(stderr, "While reading format capabilities : %s\n",
			strerror(error));
		return error;
	}

	*len = list_length;
	return 0;
}


static void
print_format(int format_tp, uint32_t num_blks, uint32_t param,
	int dscr_type, int verbose, int *supported) 
{
	char const *format_str, *nblks_str, *param_str, *user_spec;

	format_str = nblks_str = param_str = "reserved";
	user_spec = "";
	*supported = 1;

	switch (format_tp) {
	case  0x00 :
		format_str = "full format capacity";
		nblks_str  = "sectors";
		param_str  = "block length in bytes";
		user_spec  = "'-F [-b blockingnr]'";
		break;
	case  0x01 :
		format_str = "spare area expansion";
		nblks_str  = "extension in blocks";
		param_str  = "block length in bytes";
		user_spec  = "'-S'";
		break;
	/* 0x02 - 0x03 reserved */
	case  0x04 :
		format_str = "variable length zone'd format";
		nblks_str  = "zone length";
		param_str  = "zone number";
		*supported = 0;
		break;
	case  0x05 :
		format_str = "fixed length zone'd format";
		nblks_str  = "zone lenght";
		param_str  = "last zone number";
		*supported = 0;
		break;
	/* 0x06 - 0x0f reserved */
	case  0x10 :
		format_str = "CD-RW/DVD-RW full packet format";
		nblks_str  = "adressable blocks";
		param_str  = "fixed packet size/ECC blocksize in sectors";
		user_spec  = "'-F -p [-b blockingnr]'";
		break;
	case  0x11 :
		format_str = "CD-RW/DVD-RW grow session";
		nblks_str  = "adressable blocks";
		param_str  = "fixed packet size/ECC blocksize in sectors";
		user_spec  = "'-G'";
		break;
	case  0x12 :
		format_str = "CD-RW/DVD-RW add session";
		nblks_str  = "adressable blocks";
		param_str  = "maximum fixed packet size/ECC blocksize "
			     "in sectors";
		*supported = 0;
		break;
	case  0x13 :
		format_str = "DVD-RW max growth of last complete session";
		nblks_str  = "adressable blocks";
		param_str  = "ECC blocksize in sectors";
		user_spec  = "'-G'";
		break;
	case  0x14 :
		format_str = "DVD-RW quick grow last session";
		nblks_str  = "adressable blocks";
		param_str  = "ECC blocksize in sectors";
		*supported = 0;
		break;
	case  0x15 :
		format_str = "DVD-RW quick full format";
		nblks_str  = "adressable blocks";
		param_str  = "ECC blocksize in sectors";
		*supported = 0;
		break;
	/* 0x16 - 0x23 reserved */
	case  0x24 :
		format_str = "background MRW format";
		nblks_str  = "Defect Management Area blocks";
		param_str  = "not used";
		user_spec  = "'[-R] [-s] [-w] -F -M [-b blockingnr]'";
		break;
	/* 0x25 reserved */
	case  0x26 :
		format_str = "background DVD+RW full format";
		nblks_str  = "sectors";
		param_str  = "not used";
		user_spec  = "'[-R] [-w] -F'";
		break;
	/* 0x27 - 0x2f reserved */
	case  0x30 :
		format_str = "BD-RE full format with spare area";
		nblks_str  = "blocks";
		param_str  = "total spare area size in clusters";
		user_spec  = "'[-s] -F'";
		break;
	case  0x31 :
		format_str = "BD-RE full format without spare area";
		nblks_str  = "blocks";
		param_str  = "block length in bytes";
		user_spec  = "'-F'";
		break;
	/* 0x32 - 0x3f reserved */
	default :
		break;
	}

	if (verbose) {
		printf("\n\tFormat type 0x%02x : %s\n", format_tp, format_str);

		switch (dscr_type) {
		case  1 :
			printf( "\t\tUnformatted media,"
				"maximum formatted capacity\n");
			break;
		case  2 :
			printf( "\t\tFormatted media,"
				"current formatted capacity\n");
			break;
		case  3 :
			printf( "\t\tNo media present or incomplete session, "
				"maximum formatted capacity\n");
			break;
		default :
			printf("\t\tUnspecified descriptor type\n");
			break;
		}

		printf("\t\tNumber of blocks : %12d\t(%s)\n",
			num_blks, nblks_str);
		printf("\t\tParameter        : %12d\t(%s)\n",
			param, param_str);

		if (format_tp == 0x24) {
			printf( "\t\tExpert select    : "
				"'-X 0x%02x:0xffffff:0' or "
				"'-X 0x%02x:0xffff0000:0'\n",
				format_tp, format_tp);
		} else {
			printf( "\t\tExpert select    : "
				"'-X 0x%02x:%d:%d'\n",
				format_tp, num_blks, param);
		}
		if (*supported) {
			printf("\t\tmmc_format arg   : %s\n", user_spec);
		} else {
			printf("\t\t** not supported **\n");
		}
	}
}


static void
process_format_caps(uint8_t *buf, int list_length, int verbose,
	uint8_t *allow, uint32_t *blks, uint32_t *params)
{
	uint32_t	num_blks, param;
	uint8_t	       *fcd;
	int		dscr_type, format_tp;
	int		supported;

	bzero(allow,  255);
	bzero(blks,   255*4);
	bzero(params, 255*4);
	
	fcd = buf + 4;
	list_length -= 4;		/* strip header */

	if (verbose)
		printf("\tCurrent/max capacity followed by additional capacity,"
			"reported length of %d bytes (8/entry)\n", list_length);

	while (list_length > 0) {
		num_blks    = fcd[ 3]        | (fcd[ 2] << 8) |
			     (fcd[ 1] << 16) | (fcd[ 0] << 24);
		dscr_type   = fcd[ 4] & 3;
		format_tp   = fcd[ 4] >> 2;
		param       = fcd[ 7] | (fcd[ 6] << 8) |  (fcd[ 5] << 16);

		print_format(format_tp, num_blks, param, dscr_type, verbose,
			&supported);

		 allow[format_tp] = 1;	/* TODO = supported? */
		  blks[format_tp] = num_blks;
		params[format_tp] = param;

		fcd += 8;
		list_length-=8;
	}
}



/* format a CD-RW disc */
/* old style format 7 */
static int
uscsi_format_cdrw_mode7(struct uscsi_dev *mydev, uint32_t blocks)
{
	scsicmd cmd;
	struct uscsi_sense sense;
	uint8_t  buffer[16];
	int error;

	if (blocks % 32) {
		blocks -= blocks % 32;
	}

	bzero(cmd, SCSI_CMD_LEN);
	bzero(buffer, sizeof(buffer));

	cmd[0] = 0x04;			/* format unit 			   */
	cmd[1] = 0x17;			/* parameter list format 7 follows */
	cmd[5] = 0;			/* control			   */

	/* format list header */
	buffer[ 0] = 0;			/* reserved			   */
	buffer[ 1] = 0x80 | 0x02;	/* Valid info, immediate return	   */
	buffer[ 2] = 0;			/* MSB format descriptor length	   */
	buffer[ 3] = 8;			/* LSB ...			   */

	/*
	 * for CD-RW the initialisation pattern bit is reserved, but there IS
	 * one
	 */

	buffer[ 4] = 0;			/* no header			   */
	buffer[ 5] = 0;			/* default pattern		   */
	buffer[ 6] = 0;			/* pattern length MSB		   */
	buffer[ 7] = 0;			/* pattern length LSB		   */

	/* 8 bytes of format descriptor */
	/* (s)ession bit 1<<7, (g)row bit 1<<6  */
	/* SG	action	*/
	/* 00	format disc with number of user data blocks	*/
	/* 10	create new session with number of data blocks	*/
	/* x1	grow session to be number of data blocks	*/

	buffer[ 8] = 0x00;		/* session and grow bits (7 and 6)  */
	buffer[ 9] = 0;			/* reserved */
	buffer[10] = 0;			/* reserved */
	buffer[11] = 0;			/* reserved */
	buffer[12] = (blocks >> 24) & 0xff;	/* blocks MSB	*/
	buffer[13] = (blocks >> 16) & 0xff;
	buffer[14] = (blocks >>  8) & 0xff;
	buffer[15] = (blocks      ) & 0xff;	/* blocks LSB	*/

	/* this will take a while .... */
	error = uscsi_command(SCSI_WRITECMD, mydev,
			cmd, 6, buffer, sizeof(buffer), UINT_MAX, &sense);
	if (error)
		return error;

	uscsi_waitop(mydev);
	return 0;
}


static int
uscsi_format_disc(struct uscsi_dev *mydev, int immed, int format_type,
	uint32_t blocks, uint32_t param, int certification, int cmplist) 
{
	scsicmd cmd;
	struct uscsi_sense sense;
	uint8_t  buffer[16], fmt_flags;
	int error;

	fmt_flags = 0x80;		/* valid info flag */
	if (immed)
		fmt_flags |= 2;
	if (certification == 0)
		fmt_flags |= 32;

	if (cmplist)
		cmplist = 8;

#if 0
	if (mmc_profile != 0x43) {
		/* certification specifier only valid for BD-RE */
		certification = 0;
	}
#endif

	bzero(cmd, SCSI_CMD_LEN);
	bzero(buffer, sizeof(buffer));

	cmd[0] = 0x04;			/* format unit 			   */
	cmd[1] = 0x11 | cmplist;	/* parameter list format 1 follows */
	cmd[5] = 0;			/* control			   */

	/* format list header */
	buffer[ 0] = 0;			/* reserved			   */
	buffer[ 1] = 0x80 | fmt_flags;	/* Valid info, flags follow	   */
	buffer[ 2] = 0;			/* MSB format descriptor length    */
	buffer[ 3] = 8;			/* LSB ...			   */

	/* 8 bytes of format descriptor */
	buffer[ 4] = (blocks >> 24) & 0xff;	/* blocks MSB	     */
	buffer[ 5] = (blocks >> 16) & 0xff;
	buffer[ 6] = (blocks >>  8) & 0xff;
	buffer[ 7] = (blocks      ) & 0xff;	/* blocks LSB	     */
	buffer[ 8] = (format_type << 2) | certification;
	buffer[ 9] = (param  >> 16) & 0xff;	/* parameter MSB     */
	buffer[10] = (param  >>  8) & 0xff;	/*	packet size  */
	buffer[11] = (param       ) & 0xff;	/* parameter LSB     */

	/* this will take a while .... */
	error = uscsi_command(SCSI_WRITECMD, mydev,
			cmd, 6, buffer, 12, UINT_MAX, &sense);
	if (error)
		return error;

	if (immed)
		uscsi_waitop(mydev);

	return 0;
}


static int
uscsi_blank_disc(struct uscsi_dev *mydev)
{
	scsicmd cmd;
	int error;

	/* XXX check if the device can blank! */


	/* blank disc */
	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0xA1;			/* blank			*/
	cmd[ 1] = 16;			/* Immediate, blank complete	*/
	cmd[11] = 0;			/* control			*/

	/* this will take a while .... */
	error = uscsi_command(SCSI_WRITECMD, mydev,
			cmd, 12, NULL, 0, UINT_MAX, NULL);
	if (error)
		return error;

	uscsi_waitop(mydev);
	return 0;
}


static int
usage(char *program)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s [options] devicename\n", program);
	fprintf(stderr,
	"-B		blank cd-rw disc before formatting\n"
	"-F		format cd-rw disc\n"
	"-O		CD-RW formatting 'old-style' for old CD-RW drives\n"
	"-M		select MRW format\n"
	"-R		restart MRW & DVD+RW format\n"
	"-G		grow last CD-RW/DVD-RW session\n"
	"-S		grow spare space DVD-RAM/BD-RE\n"
	"-s		format DVD+MRW/BD-RE with extra spare space\n"
	"-w		wait until completion of background format\n"
	"-p		explicitly set packet format\n"
	"-c num		media certification for DVD-RAM/BD-RE : "
			"0 no, 1 full, 2 quick\n"
	"-r		recompile defect list for DVD-RAM (cmplist)\n"
	"-h -H -I	help/inquiry formats\n"
	"-X format	expert format selector form 'fmt:blks:param' with -c\n"
	"-b blockingnr	in sectors (for CD-RW)\n"
	"-D 		verbose SCSI command errors\n"
	);
	return 1;
}


extern char	*optarg;
extern int	 optind;
extern int	 optreset;


int
main(int argc, char *argv[])
{
	struct uscsi_addr saddr;
	uint32_t blks[256], params[256];
	uint32_t format_type, format_blks, format_param, blockingnr;
	uint8_t  allow[256];
	uint8_t caps[512];
	uint32_t caps_len = sizeof(caps);
	char *progname;
	int blank, format, mrw, background;
	int inquiry, spare, oldtimer;
	int expert;
	int restart_format, grow_session, grow_spare, packet_wr;
	int mmc_profile, flag, error, display_usage;
	int certification, cmplist;
	int wait_until_finished;
	progname = strdup(argv[0]);
	if (argc == 1) {
		return usage(progname);
	}

	blank               = 0;
	format              = 0;
	mrw                 = 0;
	restart_format      = 0;
	grow_session        = 0;
	grow_spare          = 0;
	wait_until_finished = 0;
	packet_wr           = 0;
	certification       = 1;
	cmplist             = 0;
	inquiry             = 0;
	spare               = 0;
	inquiry             = 0;
	oldtimer            = 0;
	expert              = 0;
	display_usage       = 0;
	blockingnr          = 32;
	uscsilib_verbose    = 0;
	while ((flag = getopt(argc, argv, "BFMRGSwpsc:rhHIX:Ob:D")) != -1) {
		switch (flag) {
			case 'B' :
				blank = 1;
				break;
			case 'F' :
				format = 1;
				break;
			case 'M' :
				mrw = 1;
				break;
			case 'R' :
				restart_format = 1;
				break;
			case 'G' :
				grow_session = 1;
				break;
			case 'S' :
				grow_spare = 1;
				break;
			case 'w' :
				wait_until_finished = 1;
				break;
			case 'p' :
				packet_wr = 1;
				break;
			case 's' :
				spare = 1;
				break;
			case 'c' :
				certification = atoi(optarg);
				break;
			case 'r' :
				cmplist = 1;
				break;
			case 'h' :
			case 'H' :
				display_usage = 1;
			case 'I' :
				inquiry = 1;
				break;
			case 'X' :
				/* TODO parse expert mode string */
				printf("-X not implemented yet\n");
				expert = 1;
				exit(1);
				break;
			case 'O' :
				/* oldtimer CD-RW format */
				oldtimer = 1;
				format   = 1;
				break;
			case 'b' :
				blockingnr = atoi(optarg);
				break;
			case 'D' :
				uscsilib_verbose = 1;
				break;
			default :
				return usage(progname);
		}
	}
	argv += optind;
	argc -= optind;

	if ((!blank && !format && !grow_session && !grow_spare) &&
	    (!expert && !inquiry)) {
		fprintf(stderr, "%s : at least one of -B, -F, -G, -S, -X or -I "
				"needs to be specified\n\n", progname);
		return usage(progname);
	}

	if (format + grow_session + grow_spare + expert > 1) {
		fprintf(stderr, "%s : at most one of -F, -G, -S or -X "
				"needs to be specified\n\n", progname);
		return usage(progname);
	}

	if (argc != 1) return usage(progname);

	/* Open the device */
	dev.dev_name = strdup(*argv);
	printf("Opening device %s\n", dev.dev_name);
	error = uscsi_open(&dev);
	if (error) {
		fprintf(stderr, "Device failed to open : %s\n",
			strerror(error));
		exit(1);
	}

	error = uscsi_check_for_scsi(&dev);
	if (error) {
		fprintf(stderr, "sorry, not a SCSI/ATAPI device : %s\n",
			strerror(error));
		exit(1);
	}

	error = uscsi_identify(&dev, &saddr);
	if (error) {
		fprintf(stderr, "SCSI/ATAPI identify returned : %s\n",
			strerror(error));
		exit(1);
	}

	printf("\nDevice identifies itself as : ");

	if (saddr.type == USCSI_TYPE_SCSI) {
		printf("SCSI   busnum = %d, target = %d, lun = %d\n",
			saddr.addr.scsi.scbus, saddr.addr.scsi.target,
			saddr.addr.scsi.lun);
	} else {
		printf("ATAPI  busnum = %d, drive = %d\n",
			saddr.addr.atapi.atbus, saddr.addr.atapi.drive);
	}

	printf("\n");

	/* get MMC profile */
	error = uscsi_get_mmc_profile(&dev, &mmc_profile);
	if (error) {
		fprintf(stderr,
			"Can't get the disc's MMC profile because of :"
			" %s\n", strerror(error));
		fprintf(stderr, "aborting\n");
		uscsi_close(&dev);
		return 1;
	}

	/* blank disc section */
	if (blank) {
		printf("\nBlanking disc.... "); fflush(stdout);
		error = uscsi_blank_disc(&dev);

		if (error) {
			printf("fail\n"); fflush(stdout);
			fprintf(stderr,
				"Blanking failed because of : %s\n",
				strerror(error));
			uscsi_close(&dev);

			return 1;
		} else {
			printf("success!\n\n");
		}
	}

	/* re-get MMC profile */
	error = uscsi_get_mmc_profile(&dev, &mmc_profile);
	if (error) {
		fprintf(stderr,
			"Can't get the disc's MMC profile because of : %s\n",
			strerror(error));
		fprintf(stderr, "aborting\n");
		uscsi_close(&dev);
		return 1;
	}

	error = get_format_capabilities(&dev, caps, &caps_len);
	if (error)
		exit(1);

	process_format_caps(caps, caps_len, inquiry, allow, blks, params);

	format_type = 0;
	/* expert format section */
	if (expert) {
	}

	if (!format && !grow_spare && !grow_session) {
		/* we're done */
		if (display_usage)
			usage(progname);
		uscsi_close(&dev);
		exit(0);
	}

	/* normal format section */
	if (format) {
		/* get current mmc profile of disc */

		if (oldtimer && mmc_profile != 0x0a) {
			printf("Oldtimer flag only defined for CD-RW; "
				"ignored\n");
		}

		switch (mmc_profile) {
		case 0x12 :	/* DVD-RAM	*/
			format_type = 0x00;
			break;
		case 0x0a :	/* CD-RW	*/
			format_type = mrw ? 0x24 : 0x10;
			packet_wr   = 1;
			break;
		case 0x13 :	/* DVD-RW restricted overwrite */
		case 0x14 :	/* DVD-RW sequential 		*/
			format_type = 0x10;
			/*
			 * Some drives suddenly stop supporting this format
			 * type when packet_wr = 1
			 */
			packet_wr   = 0;
			break;
		case 0x1a :	/* DVD+RW	*/
			format_type = mrw ? 0x24 : 0x26;
			break;
		case 0x43 :	/* BD-RE	*/
			format_type = spare ? 0x30 : 0x31;
			break;
		default :
			fprintf(stderr, "Can't format discs of type %s\n",
				print_mmc_profile(mmc_profile));
			uscsi_close(&dev);
			exit(1);
		}
	}

	if (grow_spare) {
		switch (mmc_profile) {
		case 0x12 :	/* DVD-RAM */
		case 0x43 :	/* BD-RE   */
			format_type = 0x01;
			break;
		default :
			fprintf(stderr,
				"Can't grow spare area for discs of type %s\n",
				print_mmc_profile(mmc_profile));
			uscsi_close(&dev);
			exit(1);
		}
	}

	if (grow_session) {
		switch (mmc_profile) {
		case 0x0a :	/* CD-RW */
			format_type = 0x11;
			break;
		case 0x13 :	/* DVD-RW restricted overwrite */
		case 0x14 :	/* DVD-RW sequential ? */
			format_type = 0x13;
			break;
		default :
			uscsi_close(&dev);
			fprintf(stderr,
				"Can't grow session for discs of type %s\n",
				print_mmc_profile(mmc_profile));
			exit(1);
		}
	}

	/* check if format type is allowed */
	format_blks  = blks[format_type];
	format_param = params[format_type];
	if (!allow[format_type]) {
		if (!inquiry)
			process_format_caps(caps, caps_len, 1, allow,
				blks, params);

		printf("\n");
		fflush(stdout);
		fprintf(stderr,
			"Drive indicates it can't format with deduced format "
			"type 0x%02x\n", format_type);
		uscsi_close(&dev);
		exit(1);
	}

	if (restart_format && !((mmc_profile == 0x1a) || (format_type == 0x24)))
	{
		fprintf(stderr,
			"Format restarting only for MRW formats or DVD+RW "
			"formats\n");
		uscsi_close(&dev);
		exit(1);
	}

	if (restart_format && !wait_until_finished) {
		printf( "Warning : format restarting without waiting for it be "
			"finished is prolly not handy\n");
	}

	/* explicitly select packet write just in case */
	if (packet_wr) {
		printf("Explicitly setting packet type and blocking number\n");
		error = uscsi_set_packet_parameters(&dev, blockingnr);
		if (error) {
			fprintf(stderr,
				"Can't set packet writing and blocking number: "
				"%s\n", strerror(error));
			uscsi_close(&dev);
			exit(1);
		}
	}

	/* determine if formatting is done in the background */
	background = 0;
	if (format_type == 0x24) background = 1;
	if (format_type == 0x26) background = 1;

	/* special case format type 0x24 : MRW */
	if (format_type == 0x24) {
		format_blks  = spare ? 0xffff0000 : 0xffffffff;
		format_param = restart_format;
	}
	/* special case format type 0x26 : DVD+RW */
	if (format_type == 0x26) {
		format_param = restart_format;
	}

	/* verbose to the user */
	DEBUG(
		printf("Actual format selected: "
			"format_type 0x%02x, blks %d, param %d, "
			"certification %d, cmplist %d\n",
			format_type, format_blks, format_param,
			certification, cmplist);
	);
	printf("\nFormatting.... "); fflush(stdout);

	/* formatting time! */
	if (oldtimer) {
		error = uscsi_format_cdrw_mode7(&dev, format_blks);
		background = 0;
	} else {
		error = uscsi_format_disc(&dev, !background, format_type,
				format_blks, format_param, certification,
				cmplist);
	}

	/* what now? */
	if (error) {
		printf("fail\n"); fflush(stdout);
		fprintf(stderr, "Formatting failed because of : %s\n",
			strerror(error));
	} else {
		if (background) {
			printf("background formatting in progress\n");
			if (wait_until_finished) {
				printf("Waiting for completion ... ");
				uscsi_waitop(&dev);
			}
			/* explicitly do NOT close disc ... (for now) */
			return 0;
		} else {
			printf("success!\n\n");
		}
	}

	/* finish up */
	uscsi_close(&dev);

	return error;
}

