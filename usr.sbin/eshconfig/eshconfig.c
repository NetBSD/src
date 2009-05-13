/*	$NetBSD: eshconfig.c,v 1.8.8.1 2009/05/13 19:20:22 jym Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code contributed to The NetBSD Foundation by Kevin M. Lahey
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research 
 * Center.
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
__RCSID("$NetBSD: eshconfig.c,v 1.8.8.1 2009/05/13 19:20:22 jym Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <fcntl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dev/ic/rrunnerreg.h>
#include <dev/ic/rrunnervar.h>

/*
 * Create a simple pair of tables to map possible burst DMA values
 * to the values required by the RoadRunner.
 */

struct map_dma {
	int       value;
	u_int32_t rr_value;
};

struct map_dma read_dma_map[] =  {{0,    RR_PS_READ_DISABLE},
				  {4,    RR_PS_READ_4},
				  {16,   RR_PS_READ_16},
				  {32,   RR_PS_READ_32},
				  {64,   RR_PS_READ_64},
				  {128,  RR_PS_READ_128},
				  {256,  RR_PS_READ_256},
				  {1024, RR_PS_READ_1024},
				  {-1,    0}};

struct map_dma write_dma_map[] = {{0,    RR_PS_WRITE_DISABLE},
				  {4,    RR_PS_WRITE_4},
				  {16,   RR_PS_WRITE_16},
				  {32,   RR_PS_WRITE_32},
				  {64,   RR_PS_WRITE_64},
				  {128,  RR_PS_WRITE_128},
				  {256,  RR_PS_WRITE_256},
				  {1024, RR_PS_WRITE_1024},
				  {-1,   0}};

/*
 * The RunCode is composed of separate segments, each of which has a
 * starting address in SRAM memory (for running) and in EEPROM
 * (for storage).
 */


struct rr_seg_descr {
	u_int32_t	start_addr;
	u_int32_t	length;
	u_int32_t	ee_addr;
};

static u_int32_t	do_map __P((int, struct map_dma *));
static void 		eeprom_upload __P((const char *));
static void 		eeprom_download __P((const char *));
static u_int32_t 	rr_checksum __P((const u_int32_t *, int));
static void 		esh_tune __P((void));
static void 		esh_tune_eeprom __P((void));
static void 		esh_tuning_stats __P((void));
static void 		esh_stats __P((int));
static void 		esh_reset __P((void));
static int 		drvspec_ioctl __P((char *, int, int, int, caddr_t));
static void		usage __P((void));
int			main __P((int, char *[]));


struct	ifreq ifr;
char name[30] = "esh0";
int s;

#define RR_EE_SIZE 8192
u_int32_t eeprom[RR_EE_SIZE];
u_int32_t runcode[RR_EE_SIZE];

struct ifdrv ifd;

/* drvspec_ioctl
 * 
 * We defined a driver-specific socket ioctl to allow us to tweak
 * the characteristics of network devices.  This routine will
 * provide a shortcut to calling this routine, which would otherwise
 * require lots of costly and annoying setup.
 */

static int
drvspec_ioctl(char *lname, int fd, int cmd, int len, caddr_t data)
{
	strcpy(ifd.ifd_name, lname);
	ifd.ifd_cmd = cmd;
	ifd.ifd_len = len;
	ifd.ifd_data = data;
    
	return ioctl(fd, SIOCSDRVSPEC, (caddr_t) &ifd);
}

static void
usage()
{
	fprintf(stderr, "eshconfig -- configure Essential Communications "
		"HIPPI driver\n");
	fprintf(stderr, "-b burst size for read\n");
	fprintf(stderr, "-c burst size for write:\n");
	fprintf(stderr, "\t0 (no limit), 5, 16, 32, 64, 128, 256, 1024\n");
	fprintf(stderr, "-d download filename\n");
	fprintf(stderr, "-e write data to EEPROM\n");
	fprintf(stderr, "-m minimum bytes DMA per direction\n");
	fprintf(stderr, "-r bytes before DMA starts for read\n");
	fprintf(stderr, "-s show statistics (-ss to display only non-zero)\n");
	fprintf(stderr, "-t show tuning parameters\n");
	fprintf(stderr, "-u upload filename [not working]\n");
	fprintf(stderr, "-w bytes before DMA starts for write\n");
	fprintf(stderr, "-i interrupt delay in usecs\n");
	fprintf(stderr, "-x reset interface\n");
	exit(1);
}

/* do_map
 *
 * Map between values for burst DMA sizes and the values expected by
 * the RoadRunner chip.
 */

static u_int32_t
do_map(int value, struct map_dma *map)
{
	int i;

	for (i = 0; map[i].value != -1; i++)
		if (value == map[i].value)
			return map[i].rr_value;

	return -1;
}

/* do_map_dma
 *
 * Reverse the mapping.
 */
 
static int
do_map_dma(uint32_t value, struct map_dma *map)
{
	int i;

	for (i = 0; map[i].value != -1; i++)
		if (value == map[i].rr_value)
			return map[i].value;

	return 0;
}


int dma_thresh_read = -1;
int dma_thresh_write = -1;
int dma_min_grab = -1;
int dma_max_read = -1;
int dma_max_write = -1;

int interrupt_delay = -1;

int get_stats = 0;
int get_tuning_stats = 0;
int eeprom_write = 0;
char *eeprom_download_filename = NULL;
char *eeprom_upload_filename = NULL;
int reset = 0;

struct rr_tuning rr_tune;
struct rr_eeprom rr_eeprom;
struct rr_stats rr_stats;

int
main(argc, argv)
     int argc;
     char *argv[];
{
	int ch;

	/* Parse command-line options */

	while ((ch = getopt(argc, argv, "b:c:d:ei:m:r:stu:w:x")) != -1) {
		switch (ch) {
		case 'b':
			dma_max_read = atoi(optarg);
			break;
		case 'c':
			dma_max_write = atoi(optarg);
			break;
		case 'd':
			eeprom_download_filename = optarg;
			break;
		case 'e':
			eeprom_write++;
			break;
		case 'i':
			interrupt_delay = atoi(optarg);
			break;
		case 'm':
			dma_min_grab = atoi(optarg);
			break;
		case 'r':
			dma_thresh_read = atoi(optarg);
			break;
		case 's':
			get_stats++;
			break;
		case 't':
			get_tuning_stats++;
			break;
		case 'u':
			eeprom_upload_filename = optarg;
			break;
		case 'w':
			dma_thresh_write = atoi(optarg);
			break;
		case 'x':
			reset = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();
	if (argc == 1) {
		(void) strncpy(name, argv[0], sizeof(name));
		argc--; argv++;
	}

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "socket");

	if (eeprom_upload_filename)
		eeprom_upload(eeprom_upload_filename);

	if (eeprom_download_filename)
		eeprom_download(eeprom_download_filename);

	if (get_stats) {
		esh_stats(get_stats);
	}

    	if (drvspec_ioctl(name, s, EIOCGTUNE, sizeof(struct rr_tuning),
			  (caddr_t) &rr_tune) < 0) {
		err(1, "ioctl(EIOCGTUNE)");
	}

	if (get_tuning_stats) {
		if (get_stats)
			printf("\n");
		esh_tuning_stats();
	}

	if (eeprom_write || dma_thresh_read != -1 || 
	    dma_thresh_write != -1 ||
	    dma_min_grab != -1 ||
	    dma_max_read != -1 ||
	    dma_max_write != -1 ||
	    interrupt_delay != -1) {
		esh_tune();
	}

	if (eeprom_write)
		esh_tune_eeprom();

	if (reset)
		esh_reset();

	exit(0);
}

static void
esh_tune()
{
	dma_max_read = do_map(dma_max_read, read_dma_map);
	if (dma_max_read != -1) {
		rr_tune.rt_pci_state &= ~RR_PS_READ_MASK;
		rr_tune.rt_pci_state |= dma_max_read;
	}

	dma_max_write = do_map(dma_max_write, write_dma_map);
	if (dma_max_write != -1) {
		rr_tune.rt_pci_state &= ~RR_PS_WRITE_MASK;
		rr_tune.rt_pci_state |= dma_max_write;
	}

	if (dma_min_grab != -1) {
		if ((dma_min_grab & (RR_PS_MIN_DMA_MASK >> RR_PS_MIN_DMA_SHIFT))
		    != dma_min_grab)
			usage();
		rr_tune.rt_pci_state &= ~RR_PS_MIN_DMA_MASK;
		rr_tune.rt_pci_state |= 
			(dma_min_grab << RR_PS_MIN_DMA_SHIFT);
	}

	if (dma_thresh_write != -1) {
		if (dma_thresh_write < 1 || dma_thresh_write > RR_DW_THRESHOLD_MAX)
			usage();
		rr_tune.rt_dma_write_state &= ~RR_DW_THRESHOLD_MASK;
		rr_tune.rt_dma_write_state |= 
			dma_thresh_write << RR_DW_THRESHOLD_SHIFT;
	}

	if (dma_thresh_read != -1) {
		if (dma_thresh_read < 1 || dma_thresh_read > RR_DR_THRESHOLD_MAX)
			usage();
		rr_tune.rt_dma_read_state &= ~RR_DR_THRESHOLD_MASK;
		rr_tune.rt_dma_read_state |= 
			dma_thresh_read << RR_DR_THRESHOLD_SHIFT;
	}

	rr_tune.rt_stats_timer = ESH_STATS_TIMER_DEFAULT;

	if (interrupt_delay != -1)
		rr_tune.rt_interrupt_timer = interrupt_delay;

	if (drvspec_ioctl(name, s, EIOCSTUNE, sizeof(struct rr_tuning),
			  (caddr_t) &rr_tune) < 0)
		err(1, "EIOCSTUNE");
}

/* esh_tune_eeprom
 *
 * Store the current tuning data into the eeprom.
 */

static void
esh_tune_eeprom()
{
#define LAST (RR_EE_HEADER_CHECKSUM / RR_EE_WORD_LEN)
#define FIRST (RR_EE_HEADER_CHECKSUM / RR_EE_WORD_LEN)

	u_int32_t	tuning_data[LAST + 1];

	rr_eeprom.ifr_buffer = tuning_data;
	rr_eeprom.ifr_length = sizeof(tuning_data);
	rr_eeprom.ifr_offset = 0;

	if (drvspec_ioctl(name, s, EIOCGEEPROM, sizeof(struct rr_eeprom), 
			  (caddr_t) &rr_eeprom) == -1)
		err(6, "ioctl to retrieve tuning information from EEPROM");

	tuning_data[RR_EE_PCI_STATE / RR_EE_WORD_LEN] =
		rr_tune.rt_pci_state;
	tuning_data[RR_EE_DMA_WRITE_STATE / RR_EE_WORD_LEN] = 
		rr_tune.rt_dma_write_state;
	tuning_data[RR_EE_DMA_READ_STATE / RR_EE_WORD_LEN] = 
		rr_tune.rt_dma_read_state;
	tuning_data[RR_EE_INTERRUPT_TIMER / RR_EE_WORD_LEN] = rr_tune.rt_interrupt_timer;
	tuning_data[RR_EE_STATS_TIMER / RR_EE_WORD_LEN] = 
		ESH_STATS_TIMER_DEFAULT;

	tuning_data[RR_EE_HEADER_CHECKSUM / RR_EE_WORD_LEN] = 
		rr_checksum(&tuning_data[FIRST], LAST - FIRST);

	rr_eeprom.ifr_buffer = tuning_data;
	rr_eeprom.ifr_length = sizeof(tuning_data);
	rr_eeprom.ifr_offset = 0;

	if (drvspec_ioctl(name, s, EIOCSEEPROM, sizeof(struct rr_eeprom),
			  (caddr_t) &rr_eeprom) == -1)
		err(7, "ioctl to set tuning information from EEPROM");
}

/* eeprom_upload
 *
 * Upload the EEPROM from the card and store in the data file.
 */

static void
eeprom_upload(const char *filename)
{
	int fd;

	bzero(eeprom, sizeof(eeprom));
	if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) < 0)
		err(4, "Couldn't open %s for output", filename);

	rr_eeprom.ifr_buffer = eeprom;
	rr_eeprom.ifr_length = sizeof(eeprom);
	rr_eeprom.ifr_offset = 0;

	if (drvspec_ioctl(name, s, EIOCGEEPROM, sizeof(struct rr_eeprom),
			  (caddr_t) &rr_eeprom) == -1)
		err(5, "ioctl to retrieve all of EEPROM");

	write(fd, eeprom, sizeof(eeprom));
	close(fd);
}

/* eeprom_download
 * 
 * Download into eeprom the contents of a file.  The file is made up
 * of ASCII text;  the first three characters can be ignored, the next
 * four hex characters define an address, the next two characters can
 * be ignored, and the final eight hex characters are the data.
 */

static void
eeprom_download(const char *filename)
{
	FILE *fp;
	struct rr_seg_descr *segd = NULL, *nsegd;
	char id[BUFSIZ];
	char pad[BUFSIZ];
	char buffer[BUFSIZ];
	u_int32_t address = 0;
	u_int32_t last_address = 0;
	u_int32_t value;
	u_int32_t length = 0;
	int segment_start = 0;
	int seg_table_start;
	int seg_count_offset;
	int phase2_start;
	int phase2_checksum;
	int in_segment = 0;
	int segment = 0;
	int eof = 0;
	int line = 0;
	int zero_count = 0;
	int i;

	/* Clear out eeprom storage space, then read in the value on the card */

	bzero(eeprom, sizeof(eeprom));
	bzero(runcode, sizeof(runcode));

	rr_eeprom.ifr_buffer = eeprom;
	rr_eeprom.ifr_length = sizeof(eeprom);
	rr_eeprom.ifr_offset = 0;
	if (drvspec_ioctl(name, s, EIOCGEEPROM, sizeof(struct rr_eeprom),
			  (caddr_t) &rr_eeprom) == -1)
		err(5, "ioctl to retrieve EEPROM");

	/* 
	 * Open the input file and proceed to read the data file, storing
	 * the data and counting the number of segments.
	 */

	if ((fp = fopen(filename, "r")) == NULL)
		err(2, "fopen");

	do {
		if (fgets(buffer, sizeof(buffer), fp) == NULL)
			errx(3, "premature, unmarked end of file, line %d", line);
		line++;

		if (!strncmp(buffer + 7, "01", 2)) { /* check for EOF marker... */
			eof = 1;
		} else {
			sscanf(buffer, "%3s%4x%2s%8x%2s", 
			       id, &address, pad, &value, pad);
			if (strcmp(id, ":04") != 0)
				errx(3, "bad initial id on line %d", line);
		}

		/*
		 * Check to see if we terminated a segment;  this happens
		 * when we are at end of file, or we hit a non-sequential
		 * address value, or we see three or more zeroes in a row.
		 */

		if ((length == RR_EE_SEG_SIZE || eof || zero_count >= 3 ||
		     (last_address && last_address != address - 1)) && in_segment) {

			length -= zero_count;
			segment_start += length;
			segd[segment].length = length;

			printf("segment %d, %d words\n", segment, length);
			last_address = in_segment = zero_count = length = 0;
			segment++;
		}

		if (eof)
			break;

		/* Skip zero values starting a segment */

		if (!in_segment && value == 0)
			continue;
		last_address = address;

		/* 
		 * If we haven't started a segment yet, do so now.
		 * Store away the address at which this code should be placed
		 * in memory and the address of the code in the EEPROM.
		 */

		if (!in_segment) {
			in_segment = 1;

			nsegd = realloc(segd, sizeof(struct rr_seg_descr) * (segment + 1));
			if (nsegd == NULL)
				err(6, "couldn't realloc segment descriptor space");
			segd = nsegd;

			segd[segment].start_addr = address * sizeof(u_int32_t);
			segd[segment].ee_addr = segment_start;
		}

		/* Keep track of consecutive zeroes */

		if (in_segment && value == 0)
			zero_count++;
		else 
			zero_count = 0;

		/* Store away the actual data */

		runcode[segment_start + length++] = value;
	} while (!eof);
	fclose(fp);

	/* Now that we have a segment count, fill in the EEPROM image. */

	seg_count_offset = eeprom[RR_EE_RUNCODE_SEGMENTS / RR_EE_WORD_LEN];
	seg_count_offset = (seg_count_offset - RR_EE_OFFSET) / RR_EE_WORD_LEN;
	seg_table_start = seg_count_offset + 1;
	phase2_checksum = seg_table_start + 3 * segment;
	phase2_start = eeprom[RR_EE_PHASE2_EE_START / RR_EE_WORD_LEN];
	phase2_start = (phase2_start - RR_EE_OFFSET) / RR_EE_WORD_LEN;

	printf("segment table start = %x, segments = %d\n", 
	       seg_table_start, eeprom[seg_count_offset]);

	/* We'll fill in anything after the segment count, so clear it */

	bzero(eeprom + seg_count_offset, 
	      sizeof(eeprom) - seg_count_offset * sizeof(eeprom[0]));

	eeprom[seg_count_offset] = segment;

	for (i = 0; i < segment; i++)
		segd[i].ee_addr = RR_EE_OFFSET + 
			(segd[i].ee_addr + phase2_checksum + 1) * RR_EE_WORD_LEN;

	bcopy(segd, &eeprom[seg_table_start], 
	      sizeof(struct rr_seg_descr) * segment);

	bcopy(runcode, &eeprom[phase2_checksum + 1],
	      segment_start * sizeof(u_int32_t));

	eeprom[phase2_checksum] = rr_checksum(&eeprom[phase2_start],
					      phase2_checksum - phase2_start);

	eeprom[segment_start + phase2_checksum + 1] = 
		rr_checksum(&eeprom[phase2_checksum + 1], segment_start);

	printf("phase2 checksum %x, runcode checksum %x\n",
	       eeprom[phase2_checksum], 
	       eeprom[segment_start + phase2_checksum + 1]);

	rr_eeprom.ifr_buffer = eeprom;
	rr_eeprom.ifr_length = sizeof(eeprom);
	rr_eeprom.ifr_offset = 0;
	if (drvspec_ioctl(name, s, EIOCSEEPROM, sizeof(struct rr_eeprom),
			  (caddr_t) &rr_eeprom) == -1)
		err(5, "ioctl to retrieve EEPROM");
}

/* rr_checksum
 *
 * Perform checksum on RunCode.  Length is in words.  Ugh.
 */

static u_int32_t
rr_checksum(const u_int32_t *data, int length)
{
	u_int32_t checksum = 0;

	while (length--)
		checksum += *data++;

	checksum = 0 - checksum;
	return checksum;
}

struct stats_values {
	int	 offset;
	const char *name;
};

struct stats_values stats_values[] = {
	{0x04, "receive rings created"},
	{0x08, "receive rings deleted"},
	{0x0c, "interrupts"},
	{0x10, "event overflows"},
	{0x14, "invalid commands"},
	{0x18, "DMA read errors"},
	{0x1c, "DMA write errors"},
	{0x20, "stats updates per timer"},
	{0x24, "stats updates per host"},
	{0x28, "watchdog"},
	{0x2c, "trace"},
	{0x30, "link ready sync established"},
	{0x34, "GLink errors"},
	{0x38, "alternating flag errors"},
	{0x3c, "overhead bit 8 synchronized"},
	{0x40, "remote serial parity errors"},
	{0x44, "remote parallel parity errors"},
	{0x48, "remote loopback requested"},
	{0x50, "transmit connections established"},
	{0x54, "transmit connections rejected"},
	{0x58, "transmit connections retried"},
	{0x5c, "transmit connections timed out"},
	{0x60, "transmit connections disconnected"},
	{0x64, "transmit parity errors"},
	{0x68, "packets sent"},
	{0x74, "short first burst sent"},
	{0x80, "transmit data not moving"},
	{0x90, "receive connections accepted"},
	{0x94, "receive connections rejected -- bad parity"},
	{0x98, "receive connections rejected -- 64-bit width"},
	{0x9c, "receive connections rejected -- buffers low"},
	{0xa0, "receive connections disconnected"},
	{0xa4, "receive connections with no data"},
	{0xa8, "packets received"},
	{0xb4, "short first burst received"},
	{0xc0, "receive parity error"},
	{0xc4, "receive LLRC error"},
	{0xc8, "receive burst size error"},
	{0xcc, "receive state error"},
	{0xd0, "receive ready ULP"},
	{0xd4, "receive invalid ULP"},
	{0xd8, "receive packets flow control due to buffer space"},
	{0xdc, "receive packets flow control due to descriptors"},
	{0xe0, "receive ring fulls"},
	{0xe4, "packet length errors"},
	{0xe8, "packets with checksum error"},
	{0xec, "packets dropped"},
	{0xf0, "ring low on space"},
	{0xf4, "data in ring at close"},
	{0xf8, "receives to ring not moving data"},
	{0xfc, "receiver idles"},
	{0, 0},
};

static void
esh_reset()
{
	if (drvspec_ioctl(name, s, EIOCRESET, 0, 0) < 0)
		err(1, "ioctl(EIOCRESET)");
}

static void
esh_stats(int lget_stats)
{
	u_int32_t	*stats;
	long long value;
	int offset;

	if (drvspec_ioctl(name, s, EIOCGSTATS, sizeof(struct rr_stats),
			  (caddr_t) &rr_stats) < 0)
		err(1, "ioctl(EIOCGTUNE)");

	stats = rr_stats.rs_stats;

	value = (((long long) stats[0x78 / 4]) << 32) | stats[0x7c / 4];
	if (lget_stats == 1 || value > 0)
		printf("%12lld bytes sent\n", value);
	value = ((long long) stats[0xb8 / 4] << 32) | stats[0xbc / 4];
	if (lget_stats == 1 || value > 0)
		printf("%12lld bytes received\n", value);

	for (offset = 0; stats_values[offset].offset != 0; offset++) {
		if (lget_stats == 1 || stats[stats_values[offset].offset / 4] > 0)
			printf("%12d %s\n", stats[stats_values[offset].offset / 4],
			       stats_values[offset].name);
	}
}


static void
esh_tuning_stats()
{
	printf("rt_mode_and_status = %x\n", 
	       rr_tune.rt_mode_and_status);
	printf("rt_conn_retry_count = %x\n", 
	       rr_tune.rt_conn_retry_count);
	printf("rt_conn_retry_timer = %x\n", 
	       rr_tune.rt_conn_retry_timer);
	printf("rt_conn_timeout = %x\n", rr_tune.rt_conn_timeout);
	printf("rt_stats_timer = %x\n", rr_tune.rt_stats_timer);
	printf("rt_interrupt_timer = %x\n", 
	       rr_tune.rt_interrupt_timer);
	printf("rt_tx_timeout = %x\n", rr_tune.rt_tx_timeout);
	printf("rt_rx_timeout = %x\n", rr_tune.rt_rx_timeout);
	printf("rt_pci_state = %x"
	       "     min DMA %x  read max %x write max %x\n", 
	       rr_tune.rt_pci_state,
	       (rr_tune.rt_pci_state & RR_PS_MIN_DMA_MASK) 
	       >> RR_PS_MIN_DMA_SHIFT,
	       do_map_dma(rr_tune.rt_pci_state & RR_PS_READ_MASK,
			  read_dma_map),
	       do_map_dma(rr_tune.rt_pci_state & RR_PS_WRITE_MASK,
			  write_dma_map));
	printf("rt_dma_write_state = %x\n", 
	       rr_tune.rt_dma_write_state);
	printf("rt_dma_read_state = %x\n", rr_tune.rt_dma_read_state);
	printf("rt_driver_param = %x\n", rr_tune.rt_driver_param);

}

