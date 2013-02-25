/*	$NetBSD: atactl.c,v 1.66.8.2 2013/02/25 00:28:03 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ken Hornstein.
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
 * atactl(8) - a program to control ATA devices.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: atactl.c,v 1.66.8.2 2013/02/25 00:28:03 tls Exp $");
#endif


#include <sys/param.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/ata/atareg.h>
#include <sys/ataio.h>

struct ata_smart_error {
	struct {
		uint8_t device_control;
		uint8_t features;
		uint8_t sector_count;
		uint8_t sector_number;
		uint8_t cylinder_low;
		uint8_t cylinder_high;
		uint8_t device_head;
		uint8_t command;
		uint8_t timestamp[4];
	} command[5];
	struct {
		uint8_t reserved;
		uint8_t error;
		uint8_t sector_count;
		uint8_t sector_number;
		uint8_t cylinder_low;
		uint8_t cylinder_high;
		uint8_t device_head;
		uint8_t status;
		uint8_t extended_error[19];
		uint8_t state;
		uint8_t lifetime[2];
	} error_data;
} __packed;

struct ata_smart_errorlog {
	uint8_t			data_structure_revision;
	uint8_t			mostrecenterror;
	struct ata_smart_error	log_entries[5];
	uint16_t		device_error_count;
	uint8_t			reserved[57];
	uint8_t			checksum;
} __packed;

struct command {
	const char *cmd_name;
	const char *arg_names;
	void (*cmd_func)(int, char *[]);
};

struct bitinfo {
	u_int bitmask;
	const char *string;
};

__dead static void	usage(void);
static void	ata_command(struct atareq *);
static void	print_bitinfo(const char *, const char *, u_int,
    const struct bitinfo *);
static void	print_bitinfo2(const char *, const char *, u_int, u_int,
    const struct bitinfo *);
static void	print_smart_status(void *, void *);
static void	print_error_entry(int, const struct ata_smart_error *);
static void	print_selftest_entry(int, const struct ata_smart_selftest *);

static void	print_error(const void *);
static void	print_selftest(const void *);

static const struct ataparams *getataparams(void);

static int	is_smart(void);

static int	fd;				/* file descriptor for device */
static const	char *dvname;			/* device name */
static char	dvname_store[MAXPATHLEN];	/* for opendisk(3) */
static const	char *cmdname;			/* command user issued */

static void	device_identify(int, char *[]);
static void	device_setidle(int, char *[]);
static void	device_idle(int, char *[]);
static void	device_apm(int, char *[]);
static void	device_checkpower(int, char *[]);
static void	device_smart(int, char *[]);
static void	device_security(int, char *[]);

static void	device_smart_temp(const struct ata_smart_attr *, uint64_t);

static const struct command device_commands[] = {
	{ "identify",	"",			device_identify },
	{ "setidle",	"idle-timer",		device_setidle },
	{ "apm",	"disable|set #",	device_apm },
	{ "setstandby",	"standby-timer",	device_setidle },
	{ "idle",	"",			device_idle },
	{ "standby",	"",			device_idle },
	{ "sleep",	"",			device_idle },
	{ "checkpower",	"",			device_checkpower },
	{ "smart",
		"enable|disable|status|offline #|error-log|selftest-log",
						device_smart },
	{ "security",
		"status|freeze|[setpass|unlock|disable|erase] [user|master]",
						device_security },
	{ NULL,		NULL,			NULL },
};

static void	bus_reset(int, char *[]);

static const struct command bus_commands[] = {
	{ "reset",	"",			bus_reset },
	{ NULL,		NULL,			NULL },
};

/*
 * Tables containing bitmasks used for error reporting and
 * device identification.
 */

static const struct bitinfo ata_caps[] = {
	{ WDC_CAP_DMA, "DMA" },
	{ WDC_CAP_LBA, "LBA" },
	{ ATA_CAP_STBY, "ATA standby timer values" },
	{ WDC_CAP_IORDY, "IORDY operation" },
	{ WDC_CAP_IORDY_DSBL, "IORDY disabling" },
	{ 0, NULL },
};

static const struct bitinfo ata_vers[] = {
	{ WDC_VER_ATA1,	"ATA-1" },
	{ WDC_VER_ATA2,	"ATA-2" },
	{ WDC_VER_ATA3,	"ATA-3" },
	{ WDC_VER_ATA4,	"ATA-4" },
	{ WDC_VER_ATA5,	"ATA-5" },
	{ WDC_VER_ATA6,	"ATA-6" },
	{ WDC_VER_ATA7,	"ATA-7" },
	{ WDC_VER_ATA8, "ATA-8" },
	{ 0, NULL },
};

static const struct bitinfo ata_cmd_set1[] = {
	{ WDC_CMD1_NOP, "NOP command" },
	{ WDC_CMD1_RB, "READ BUFFER command" },
	{ WDC_CMD1_WB, "WRITE BUFFER command" },
	{ WDC_CMD1_HPA, "Host Protected Area feature set" },
	{ WDC_CMD1_DVRST, "DEVICE RESET command" },
	{ WDC_CMD1_SRV, "SERVICE interrupt" },
	{ WDC_CMD1_RLSE, "release interrupt" },
	{ WDC_CMD1_AHEAD, "look-ahead" },
	{ WDC_CMD1_CACHE, "write cache" },
	{ WDC_CMD1_PKT, "PACKET command feature set" },
	{ WDC_CMD1_PM, "Power Management feature set" },
	{ WDC_CMD1_REMOV, "Removable Media feature set" },
	{ WDC_CMD1_SEC, "Security Mode feature set" },
	{ WDC_CMD1_SMART, "SMART feature set" },
	{ 0, NULL },
};

static const struct bitinfo ata_cmd_set2[] = {
	{ ATA_CMD2_FCE, "FLUSH CACHE EXT command" },
	{ WDC_CMD2_FC, "FLUSH CACHE command" },
	{ WDC_CMD2_DCO, "Device Configuration Overlay feature set" },
	{ ATA_CMD2_LBA48, "48-bit Address feature set" },
	{ WDC_CMD2_AAM, "Automatic Acoustic Management feature set" },
	{ WDC_CMD2_SM, "SET MAX security extension" },
	{ WDC_CMD2_SFREQ, "SET FEATURES required to spin-up after power-up" },
	{ WDC_CMD2_PUIS, "Power-Up In Standby feature set" },
	{ WDC_CMD2_RMSN, "Removable Media Status Notification feature set" },
	{ ATA_CMD2_APM, "Advanced Power Management feature set" },
	{ ATA_CMD2_CFA, "CFA feature set" },
	{ ATA_CMD2_RWQ, "READ/WRITE DMA QUEUED commands" },
	{ WDC_CMD2_DM, "DOWNLOAD MICROCODE command" },
	{ 0, NULL },
};

static const struct bitinfo ata_cmd_ext[] = {
	{ ATA_CMDE_TLCONT, "Time-limited R/W feature set R/W Continuous mode" },
	{ ATA_CMDE_TL, "Time-limited Read/Write" },
	{ ATA_CMDE_URGW, "URG bit for WRITE STREAM DMA/PIO" },
	{ ATA_CMDE_URGR, "URG bit for READ STREAM DMA/PIO" },
	{ ATA_CMDE_WWN, "World Wide Name" },
	{ ATA_CMDE_WQFE, "WRITE DMA QUEUED FUA EXT command" },
	{ ATA_CMDE_WFE, "WRITE DMA/MULTIPLE FUA EXT commands" },
	{ ATA_CMDE_GPL, "General Purpose Logging feature set" },
	{ ATA_CMDE_STREAM, "Streaming feature set" },
	{ ATA_CMDE_MCPTC, "Media Card Pass Through Command feature set" },
	{ ATA_CMDE_MS, "Media serial number" },
	{ ATA_CMDE_SST, "SMART self-test" },
	{ ATA_CMDE_SEL, "SMART error logging" },
	{ 0, NULL },
};

static const struct bitinfo ata_sata_caps[] = {
	{ SATA_SIGNAL_GEN1, "1.5Gb/s signaling" },
	{ SATA_SIGNAL_GEN2, "3.0Gb/s signaling" },
	{ SATA_SIGNAL_GEN3, "6.0Gb/s signaling" },
	{ SATA_NATIVE_CMDQ, "Native Command Queuing" },
	{ SATA_HOST_PWR_MGMT, "Host-Initiated Interface Power Management" },
	{ SATA_PHY_EVNT_CNT, "PHY Event Counters" },
	{ 0, NULL },
};

static const struct bitinfo ata_sata_feat[] = {
	{ SATA_NONZERO_OFFSETS, "Non-zero Offset DMA" },
	{ SATA_DMA_SETUP_AUTO, "DMA Setup Auto Activate" },
	{ SATA_DRIVE_PWR_MGMT, "Device-Initiated Interface Power Managment" },
	{ SATA_IN_ORDER_DATA, "In-order Data Delivery" },
	{ SATA_SW_STTNGS_PRS, "Software Settings Preservation" },
	{ 0, NULL },
};

static const struct {
	const int	id;
	const char	*name;
	void (*special)(const struct ata_smart_attr *, uint64_t);
} smart_attrs[] = {
	{   1,		"Raw read error rate", NULL },
	{   2,		"Throughput performance", NULL },
	{   3,		"Spin-up time", NULL },
	{   4,		"Start/stop count", NULL },
	{   5,		"Reallocated sector count", NULL },
	{   6,		"Read channel margin", NULL },
	{   7,		"Seek error rate", NULL },
	{   8,		"Seek time performance", NULL },
	{   9,		"Power-on hours count", NULL },
	{  10,		"Spin retry count", NULL },
	{  11,		"Calibration retry count", NULL },
	{  12,		"Device power cycle count", NULL },
	{  13,		"Soft read error rate", NULL },
	{ 187,          "Reported uncorrect", NULL },
	{ 189,          "High Fly Writes", NULL },
	{ 190,          "Airflow Temperature",		device_smart_temp },
	{ 191,		"G-sense error rate", NULL },
	{ 192,		"Power-off retract count", NULL },
	{ 193,		"Load cycle count", NULL },
	{ 194,		"Temperature",			device_smart_temp},
	{ 195,		"Hardware ECC Recovered", NULL },
	{ 196,		"Reallocated event count", NULL },
	{ 197,		"Current pending sector", NULL },
	{ 198,		"Offline uncorrectable", NULL },
	{ 199,		"Ultra DMA CRC error count", NULL },
	{ 200,		"Write error rate", NULL },
	{ 201,		"Soft read error rate", NULL },
	{ 202,		"Data address mark errors", NULL },
	{ 203,		"Run out cancel", NULL },
	{ 204,		"Soft ECC correction", NULL },
	{ 205,		"Thermal asperity check", NULL },
	{ 206,		"Flying height", NULL },
	{ 207,		"Spin high current", NULL },
	{ 208,		"Spin buzz", NULL },
	{ 209,		"Offline seek performance", NULL },
	{ 220,		"Disk shift", NULL },
	{ 221,		"G-Sense error rate", NULL },
	{ 222,		"Loaded hours", NULL },
	{ 223,		"Load/unload retry count", NULL },
	{ 224,		"Load friction", NULL },
	{ 225,		"Load/unload cycle count", NULL },
	{ 226,		"Load-in time", NULL },
	{ 227,		"Torque amplification count", NULL },
	{ 228,		"Power-off retract count", NULL },
	{ 230,		"GMR head amplitude", NULL },
	{ 231,		"Temperature",			device_smart_temp },
	{ 240,		"Head flying hours", NULL },
	{ 250,		"Read error retry rate", NULL },
	{   0,		"Unknown", NULL },
};

static const struct bitinfo ata_sec_st[] = {
	{ WDC_SEC_SUPP,		"supported" },
	{ WDC_SEC_EN,		"enabled" },
	{ WDC_SEC_LOCKED,	"locked" },
	{ WDC_SEC_FROZEN,	"frozen" },
	{ WDC_SEC_EXP,		"expired" },
	{ WDC_SEC_ESE_SUPP,	"enhanced erase support" },
	{ WDC_SEC_LEV_MAX,	"maximum level" },
	{ 0,			NULL },
};

int
main(int argc, char *argv[])
{
	int i;
	const struct command *commands = NULL;

	/* Must have at least: device command */
	if (argc < 3)
		usage();

	/* Skip program name, get and skip device name and command. */
	dvname = argv[1];
	cmdname = argv[2];
	argv += 3;
	argc -= 3;

	/*
	 * Open the device
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

	/* Look up and call the command. */
	for (i = 0; device_commands[i].cmd_name != NULL; i++) {
		if (strcmp(cmdname, device_commands[i].cmd_name) == 0) {
			commands = &device_commands[i];
			break;
		}
	}
	if (commands == NULL) {
		for (i = 0; bus_commands[i].cmd_name != NULL; i++) {
			if (strcmp(cmdname, bus_commands[i].cmd_name) == 0) {
				commands = &bus_commands[i];
				break;
			}
		}
	}
	if (commands == NULL)
		errx(1, "unknown command: %s", cmdname);

	(*commands->cmd_func)(argc, argv);
	exit(0);
}

static void
usage(void)
{
	int i;

	fprintf(stderr, "usage: %s device command [arg [...]]\n",
	    getprogname());

	fprintf(stderr, "   Available device commands:\n");
	for (i=0; device_commands[i].cmd_name != NULL; i++)
		fprintf(stderr, "\t%s %s\n", device_commands[i].cmd_name,
					    device_commands[i].arg_names);

	fprintf(stderr, "   Available bus commands:\n");
	for (i=0; bus_commands[i].cmd_name != NULL; i++)
		fprintf(stderr, "\t%s %s\n", bus_commands[i].cmd_name,
					    bus_commands[i].arg_names);

	exit(1);
}

/*
 * Wrapper that calls ATAIOCCOMMAND and checks for errors
 */

static void
ata_command(struct atareq *req)
{
	int error;

	error = ioctl(fd, ATAIOCCOMMAND, req);

	if (error == -1)
		err(1, "ATAIOCCOMMAND failed");

	switch (req->retsts) {

	case ATACMD_OK:
		return;
	case ATACMD_TIMEOUT:
		fprintf(stderr, "ATA command timed out\n");
		exit(1);
	case ATACMD_DF:
		fprintf(stderr, "ATA device returned a Device Fault\n");
		exit(1);
	case ATACMD_ERROR:
		if (req->error & WDCE_ABRT)
			fprintf(stderr, "ATA device returned Aborted "
				"Command\n");
		else
			fprintf(stderr, "ATA device returned error register "
				"%0x\n", req->error);
		exit(1);
	default:
		fprintf(stderr, "ATAIOCCOMMAND returned unknown result code "
			"%d\n", req->retsts);
		exit(1);
	}
}

/*
 * Print out strings associated with particular bitmasks
 */

static void
print_bitinfo(const char *bf, const char *af, u_int bits,
    const struct bitinfo *binfo)
{

	for (; binfo->bitmask != 0; binfo++)
		if (bits & binfo->bitmask)
			printf("%s%s%s", bf, binfo->string, af);
}

static void
print_bitinfo2(const char *bf, const char *af, u_int bits, u_int enables,
    const struct bitinfo *binfo)
{

	for (; binfo->bitmask != 0; binfo++)
		if (bits & binfo->bitmask)
			printf("%s%s (%s)%s", bf, binfo->string,
			    (enables & binfo->bitmask) ? "enabled" : "disabled",
			    af);
}


/*
 * Try to print SMART temperature field
 */

static void
device_smart_temp(const struct ata_smart_attr *attr, uint64_t raw_value)
{
	printf("%" PRIu8, attr->raw[0]);
	if (attr->raw[0] != raw_value)
		printf(" Lifetime min/max %" PRIu8 "/%" PRIu8,
		    attr->raw[2], attr->raw[4]);
}


/*
 * Print out SMART attribute thresholds and values
 */

static void
print_smart_status(void *vbuf, void *tbuf)
{
	const struct ata_smart_attributes *value_buf = vbuf;
	const struct ata_smart_thresholds *threshold_buf = tbuf;
	const struct ata_smart_attr *attr;
	uint64_t raw_value;
	int flags;
	int i, j;
	int aid;
	uint8_t checksum;

	for (i = checksum = 0; i < 512; i++)
		checksum += ((const uint8_t *) value_buf)[i];
	if (checksum != 0) {
		fprintf(stderr, "SMART attribute values checksum error\n");
		return;
	}

	for (i = checksum = 0; i < 512; i++)
		checksum += ((const uint8_t *) threshold_buf)[i];
	if (checksum != 0) {
		fprintf(stderr, "SMART attribute thresholds checksum error\n");
		return;
	}

	printf("id value thresh crit collect reliability description"
	    "\t\t\traw\n");
	for (i = 0; i < 256; i++) {
		int thresh = 0;

		attr = NULL;

		for (j = 0; j < 30; j++) {
			if (value_buf->attributes[j].id == i)
				attr = &value_buf->attributes[j];
			if (threshold_buf->thresholds[j].id == i)
				thresh = threshold_buf->thresholds[j].value;
		}

		if (thresh && attr == NULL)
			errx(1, "threshold but not attr %d", i);
		if (attr == NULL)
			continue;

		if (attr->value == 0||attr->value == 0xFE||attr->value == 0xFF)
			continue;

		for (aid = 0;
		     smart_attrs[aid].id != i && smart_attrs[aid].id != 0;
		     aid++)
			;

		flags = le16toh(attr->flags);

		printf("%3d %3d  %3d     %-3s %-7s %stive    %-24s\t",
		    i, attr->value, thresh,
		    flags & WDSM_ATTR_ADVISORY ? "yes" : "no",
		    flags & WDSM_ATTR_COLLECTIVE ? "online" : "offline",
		    attr->value > thresh ? "posi" : "nega",
		    smart_attrs[aid].name);

		for (j = 0, raw_value = 0; j < 6; j++)
			raw_value += ((uint64_t)attr->raw[j]) << (8*j);

		if (smart_attrs[aid].special)
			(*smart_attrs[aid].special)(attr, raw_value);
		else
			printf("%" PRIu64, raw_value);
		printf("\n");
	}
}

static const struct {
	int number;
	const char *name;
} selftest_name[] = {
	{ 0, "Off-line" },
	{ 1, "Short off-line" },
	{ 2, "Extended off-line" },
	{ 127, "Abort off-line test" },
	{ 129, "Short captive" },
	{ 130, "Extended captive" },
	{ 256, "Unknown test" }, /* larger then uint8_t */
	{ 0, NULL }
};

static const char *selftest_status[] = {
	"No error",
	"Aborted by the host",
	"Interrupted by the host by reset",
	"Fatal error or unknown test error",
	"Unknown test element failed",
	"Electrical test element failed",
	"The Servo (and/or seek) test element failed",
	"Read element of test failed",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Self-test in progress"
};

static void
print_error_entry(int num, const struct ata_smart_error *le)
{
	int i;

	printf("Log entry: %d\n", num);

	for (i = 0; i < 5; i++)
		printf("\tCommand %d: dc=%02x sf=%02x sc=%02x sn=%02x cl=%02x "
		    "ch=%02x dh=%02x cmd=%02x time=%02x%02x%02x%02x\n", i,
		    le->command[i].device_control,
		    le->command[i].features,
		    le->command[i].sector_count,
		    le->command[i].sector_number,
		    le->command[i].cylinder_low,
		    le->command[i].cylinder_high,
		    le->command[i].device_head,
		    le->command[i].command,
		    le->command[i].timestamp[3],
		    le->command[i].timestamp[2],
		    le->command[i].timestamp[1],
		    le->command[i].timestamp[0]);
	printf("\tError: err=%02x sc=%02x sn=%02x cl=%02x ch=%02x dh=%02x "
	    "status=%02x state=%02x lifetime=%02x%02x\n",
	    le->error_data.error,
	    le->error_data.sector_count,
	    le->error_data.sector_number,
	    le->error_data.cylinder_low,
	    le->error_data.cylinder_high,
	    le->error_data.device_head,
	    le->error_data.status,
	    le->error_data.state,
	    le->error_data.lifetime[1],
	    le->error_data.lifetime[0]);
	printf("\tExtended: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x "
	    "%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	    le->error_data.extended_error[0],
	    le->error_data.extended_error[1],
	    le->error_data.extended_error[2],
	    le->error_data.extended_error[3],
	    le->error_data.extended_error[4],
	    le->error_data.extended_error[5],
	    le->error_data.extended_error[6],
	    le->error_data.extended_error[7],
	    le->error_data.extended_error[8],
	    le->error_data.extended_error[9],
	    le->error_data.extended_error[10],
	    le->error_data.extended_error[11],
	    le->error_data.extended_error[12],
	    le->error_data.extended_error[13],
	    le->error_data.extended_error[14],
	    le->error_data.extended_error[15],
	    le->error_data.extended_error[15],
	    le->error_data.extended_error[17],
	    le->error_data.extended_error[18]);
}

static void
print_error(const void *buf)
{
	const struct ata_smart_errorlog *erlog = buf;
	uint8_t checksum;
	int i;

	for (i = checksum = 0; i < 512; i++)
		checksum += ((const uint8_t *) buf)[i];
	if (checksum != 0) {
		fprintf(stderr, "SMART error log checksum error\n");
		return;
	}

	if (erlog->data_structure_revision != 1) {
		fprintf(stderr, "Error log revision not 1 (found 0x%04x)\n",
		    erlog->data_structure_revision);
		return;
	}

	if (erlog->mostrecenterror == 0) {
		printf("No errors have been logged\n");
		return;
	}

	if (erlog->mostrecenterror > 5) {
		fprintf(stderr, "Most recent error is too large\n");
		return;
	}

	for (i = erlog->mostrecenterror; i < 5; i++)
		print_error_entry(i, &erlog->log_entries[i]);
	for (i = 0; i < erlog->mostrecenterror; i++)
		print_error_entry(i, &erlog->log_entries[i]);
	printf("device error count: %d\n", erlog->device_error_count);
}

static void
print_selftest_entry(int num, const struct ata_smart_selftest *le)
{
	const unsigned char *p;
	size_t i;

	/* check if all zero */
	for (p = (const void *)le, i = 0; i < sizeof(*le); i++)
		if (p[i] != 0)
			break;
	if (i == sizeof(*le))
		return;

	printf("Log entry: %d\n", num);

	/* Get test name */
	for (i = 0; selftest_name[i].name != NULL; i++)
		if (selftest_name[i].number == le->number)
			break;

	if (selftest_name[i].name == NULL)
		printf("\tName: (%d)\n", le->number);
	else
		printf("\tName: %s\n", selftest_name[i].name);
	printf("\tStatus: %s\n", selftest_status[le->status >> 4]);
	/* XXX This generally should not be set when a self-test is completed,
	   and at any rate is useless.  - mycroft */
	if (le->status >> 4 == 15)
		printf("\tPercent of test remaining: %1d0\n", le->status & 0xf);
	else if (le->status >> 4 != 0)
		printf("\tLBA first error: %d\n", le32toh(le->lba_first_error));
}

static void
print_selftest(const void *buf)
{
	const struct ata_smart_selftestlog *stlog = buf;
	uint8_t checksum;
	int i;

	for (i = checksum = 0; i < 512; i++)
		checksum += ((const uint8_t *) buf)[i];
	if (checksum != 0) {
		fprintf(stderr, "SMART selftest log checksum error\n");
		return;
	}

	if (le16toh(stlog->data_structure_revision) != 1) {
		fprintf(stderr, "Self-test log revision not 1 (found 0x%04x)\n",
		    le16toh(stlog->data_structure_revision));
		return;
	}

	if (stlog->mostrecenttest == 0) {
		printf("No self-tests have been logged\n");
		return;
	}

	if (stlog->mostrecenttest > 22) {
		fprintf(stderr, "Most recent test is too large\n");
		return;
	}

	for (i = stlog->mostrecenttest; i < 22; i++)
		print_selftest_entry(i, &stlog->log_entries[i]);
	for (i = 0; i < stlog->mostrecenttest; i++)
		print_selftest_entry(i, &stlog->log_entries[i]);
}

static const struct ataparams *
getataparams(void)
{
	struct atareq req;
	static union {
		unsigned char inbuf[DEV_BSIZE];
		struct ataparams inqbuf;
	} inbuf;

	memset(&inbuf, 0, sizeof(inbuf));
	memset(&req, 0, sizeof(req));

	req.flags = ATACMD_READ;
	req.command = WDCC_IDENTIFY;
	req.databuf = &inbuf;
	req.datalen = sizeof(inbuf);
	req.timeout = 1000;

	ata_command(&req);

	return (&inbuf.inqbuf);
}

/*
 * is_smart:
 *
 *	Detect whether device supports SMART and SMART is enabled.
 */

static int
is_smart(void)
{
	int retval = 0;
	const struct ataparams *inqbuf;
	const char *status;

	inqbuf = getataparams();

	if (inqbuf->atap_cmd_def != 0 && inqbuf->atap_cmd_def != 0xffff) {
		if (!(inqbuf->atap_cmd_set1 & WDC_CMD1_SMART)) {
			fprintf(stderr, "SMART unsupported\n");
		} else {
			if (inqbuf->atap_ata_major <= WDC_VER_ATA5 ||
			    inqbuf->atap_cmd_set2 == 0xffff ||
			    inqbuf->atap_cmd_set2 == 0x0000) {
				status = "status unknown";
				retval = 2;
			} else {
				if (inqbuf->atap_cmd1_en & WDC_CMD1_SMART) {
					status = "enabled";
					retval = 1;
				} else {
					status = "disabled";
					retval = 3;
				}
			}
			printf("SMART supported, SMART %s\n", status);
		}
	}
	return retval;
}

/*
 * extract_string: copy a block of bytes out of ataparams and make
 * a proper string out of it, truncating trailing spaces and preserving
 * strict typing. And also, not doing unaligned accesses.
 */
static void
extract_string(char *buf, size_t bufmax,
	       const uint8_t *bytes, size_t numbytes,
	       int needswap)
{
	unsigned i;
	size_t j;
	unsigned char ch1, ch2;

	for (i = 0, j = 0; i < numbytes; i += 2) {
		ch1 = bytes[i];
		ch2 = bytes[i+1];
		if (needswap && j < bufmax-1) {
			buf[j++] = ch2;
		}
		if (j < bufmax-1) {
			buf[j++] = ch1;
		}
		if (!needswap && j < bufmax-1) {
			buf[j++] = ch2;
		}
	}
	while (j > 0 && buf[j-1] == ' ') {
		j--;
	}
	buf[j] = '\0';
}

static void
compute_capacity(const struct ataparams *inqbuf, uint64_t *capacityp,
    uint64_t *sectorsp, uint32_t *secsizep)
{
	uint64_t capacity;
	uint64_t sectors;
	uint32_t secsize;

	if (inqbuf->atap_cmd2_en != 0 && inqbuf->atap_cmd2_en != 0xffff &&
	    inqbuf->atap_cmd2_en & ATA_CMD2_LBA48) {
		sectors =
		    ((uint64_t)inqbuf->atap_max_lba[3] << 48) |
		    ((uint64_t)inqbuf->atap_max_lba[2] << 32) |
		    ((uint64_t)inqbuf->atap_max_lba[1] << 16) |
		    ((uint64_t)inqbuf->atap_max_lba[0] <<  0);
	} else if (inqbuf->atap_capabilities1 & WDC_CAP_LBA) {
		sectors = (inqbuf->atap_capacity[1] << 16) |
		    inqbuf->atap_capacity[0];
	} else {
		sectors = inqbuf->atap_cylinders *
		    inqbuf->atap_heads * inqbuf->atap_sectors;
	}

	secsize = 512;

	if ((inqbuf->atap_secsz & ATA_SECSZ_VALID_MASK) == ATA_SECSZ_VALID) {
		if (inqbuf->atap_secsz & ATA_SECSZ_LLS) {
			secsize = 2 *		/* words to bytes */
			    (inqbuf->atap_lls_secsz[1] << 16 |
			    inqbuf->atap_lls_secsz[0] <<  0);
		}
	}

	capacity = sectors * secsize;

	if (capacityp)
		*capacityp = capacity;
	if (sectorsp)
		*sectorsp = sectors;
	if (secsizep)
		*secsizep = secsize;
}

/*
 * DEVICE COMMANDS
 */

/*
 * device_identify:
 *
 *	Display the identity of the device
 */
static void
device_identify(int argc, char *argv[])
{
	const struct ataparams *inqbuf;
	char model[sizeof(inqbuf->atap_model)+1];
	char revision[sizeof(inqbuf->atap_revision)+1];
	char serial[sizeof(inqbuf->atap_serial)+1];
	char hnum[12];
	uint64_t capacity;
	uint64_t sectors;
	uint32_t secsize;
	int lb_per_pb;
	int needswap = 0;
	int i;
	uint8_t checksum;

	/* No arguments. */
	if (argc != 0)
		usage();

	inqbuf = getataparams();

	if ((inqbuf->atap_integrity & WDC_INTEGRITY_MAGIC_MASK) ==
	    WDC_INTEGRITY_MAGIC) {
		for (i = checksum = 0; i < 512; i++)
			checksum += ((const uint8_t *)inqbuf)[i];
		if (checksum != 0)
			puts("IDENTIFY DEVICE data checksum invalid\n");
	}

#if BYTE_ORDER == LITTLE_ENDIAN
	/*
	 * On little endian machines, we need to shuffle the string
	 * byte order.  However, we don't have to do this for NEC or
	 * Mitsumi ATAPI devices
	 */

	if (!((inqbuf->atap_config & WDC_CFG_ATAPI_MASK) == WDC_CFG_ATAPI &&
	      ((inqbuf->atap_model[0] == 'N' &&
		  inqbuf->atap_model[1] == 'E') ||
	       (inqbuf->atap_model[0] == 'F' &&
		  inqbuf->atap_model[1] == 'X')))) {
		needswap = 1;
	}
#endif

	/*
	 * Copy the info strings out, stripping off blanks.
	 */
	extract_string(model, sizeof(model),
		inqbuf->atap_model, sizeof(inqbuf->atap_model),
		needswap);
	extract_string(revision, sizeof(revision),
		inqbuf->atap_revision, sizeof(inqbuf->atap_revision),
		needswap);
	extract_string(serial, sizeof(serial),
		inqbuf->atap_serial, sizeof(inqbuf->atap_serial),
		needswap);

	printf("Model: %s, Rev: %s, Serial #: %s\n",
		model, revision, serial);

	if (inqbuf->atap_cmd_ext != 0 && inqbuf->atap_cmd_ext != 0xffff &&
	    inqbuf->atap_cmd_ext & ATA_CMDE_WWN)
		printf("World Wide Name: %016" PRIX64 "\n",
		    ((uint64_t)inqbuf->atap_wwn[0] << 48) |
		    ((uint64_t)inqbuf->atap_wwn[1] << 32) |
		    ((uint64_t)inqbuf->atap_wwn[2] << 16) |
		    ((uint64_t)inqbuf->atap_wwn[3] <<  0));

	printf("Device type: %s, %s\n", inqbuf->atap_config & WDC_CFG_ATAPI ?
	       "ATAPI" : "ATA", inqbuf->atap_config & ATA_CFG_FIXED ? "fixed" :
	       "removable");

	compute_capacity(inqbuf, &capacity, &sectors, &secsize);

	humanize_number(hnum, sizeof(hnum), capacity, "bytes",
		HN_AUTOSCALE, HN_DIVISOR_1000);

	printf("Capacity %s, %" PRIu64 " sectors, %" PRIu32 " bytes/sector\n",
		       hnum, sectors, secsize);

	printf("Cylinders: %d, heads: %d, sec/track: %d\n",
		inqbuf->atap_cylinders, inqbuf->atap_heads,
		inqbuf->atap_sectors);

	lb_per_pb = 1;

	if ((inqbuf->atap_secsz & ATA_SECSZ_VALID_MASK) == ATA_SECSZ_VALID) {
		if (inqbuf->atap_secsz & ATA_SECSZ_LPS) {
			lb_per_pb <<= inqbuf->atap_secsz & ATA_SECSZ_LPS_SZMSK;
			printf("Physical sector size: %d bytes\n",
			    lb_per_pb * secsize);
			if ((inqbuf->atap_logical_align &
			    ATA_LA_VALID_MASK) == ATA_LA_VALID) {
				printf("First physically aligned sector: %d\n",
				    lb_per_pb - (inqbuf->atap_logical_align &
					ATA_LA_MASK));
			}
		}
	}

	if (((inqbuf->atap_sata_caps & SATA_NATIVE_CMDQ) ||
	    (inqbuf->atap_cmd_set2 & ATA_CMD2_RWQ)) &&
	    (inqbuf->atap_queuedepth & WDC_QUEUE_DEPTH_MASK))
		printf("Command queue depth: %d\n",
		    (inqbuf->atap_queuedepth & WDC_QUEUE_DEPTH_MASK) + 1);

	printf("Device capabilities:\n");
	print_bitinfo("\t", "\n", inqbuf->atap_capabilities1, ata_caps);

	if (inqbuf->atap_ata_major != 0 && inqbuf->atap_ata_major != 0xffff) {
		printf("Device supports following standards:\n");
		print_bitinfo("", " ", inqbuf->atap_ata_major, ata_vers);
		printf("\n");
	}

	if (inqbuf->atap_cmd_set1 != 0 && inqbuf->atap_cmd_set1 != 0xffff &&
	    inqbuf->atap_cmd_set2 != 0 && inqbuf->atap_cmd_set2 != 0xffff) {
		printf("Command set support:\n");
		if (inqbuf->atap_cmd1_en != 0 && inqbuf->atap_cmd1_en != 0xffff)
			print_bitinfo2("\t", "\n", inqbuf->atap_cmd_set1,
			    inqbuf->atap_cmd1_en, ata_cmd_set1);
		else
			print_bitinfo("\t", "\n", inqbuf->atap_cmd_set1,
			    ata_cmd_set1);
		if (inqbuf->atap_cmd2_en != 0 && inqbuf->atap_cmd2_en != 0xffff)
			print_bitinfo2("\t", "\n", inqbuf->atap_cmd_set2,
			    inqbuf->atap_cmd2_en, ata_cmd_set2);
		else
			print_bitinfo("\t", "\n", inqbuf->atap_cmd_set2,
			    ata_cmd_set2);
		if (inqbuf->atap_cmd_ext != 0 && inqbuf->atap_cmd_ext != 0xffff)
			print_bitinfo("\t", "\n", inqbuf->atap_cmd_ext,
			    ata_cmd_ext);
	}

	if (inqbuf->atap_sata_caps != 0 && inqbuf->atap_sata_caps != 0xffff) {
		printf("Serial ATA capabilities:\n");
		print_bitinfo("\t", "\n",
		    inqbuf->atap_sata_caps, ata_sata_caps);

	}

	if (inqbuf->atap_sata_features_supp != 0 &&
	    inqbuf->atap_sata_features_supp != 0xffff) {
		printf("Serial ATA features:\n");
		if (inqbuf->atap_sata_features_en != 0 &&
		    inqbuf->atap_sata_features_en != 0xffff)
			print_bitinfo2("\t", "\n",
			    inqbuf->atap_sata_features_supp,
			    inqbuf->atap_sata_features_en, ata_sata_feat);
		else
			print_bitinfo("\t", "\n",
			    inqbuf->atap_sata_features_supp, ata_sata_feat);
	}

	if ((inqbuf->atap_ata_major & WDC_VER_ATA8) &&
	    (inqbuf->support_dsm & ATA_SUPPORT_DSM_TRIM))
		printf("TRIM supported\n");

	return;
}

/*
 * device idle:
 *
 * issue the IDLE IMMEDIATE command to the drive
 */
static void
device_idle(int argc, char *argv[])
{
	struct atareq req;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&req, 0, sizeof(req));

	if (strcmp(cmdname, "idle") == 0)
		req.command = WDCC_IDLE_IMMED;
	else if (strcmp(cmdname, "standby") == 0)
		req.command = WDCC_STANDBY_IMMED;
	else
		req.command = WDCC_SLEEP;

	req.timeout = 1000;

	ata_command(&req);

	return;
}

/*
 * device apm:
 *
 * enable/disable/control the APM feature of the drive
 */
static void
device_apm(int argc, char *argv[])
{
	struct atareq req;
	long l;

	memset(&req, 0, sizeof(req));
	if (argc >= 1) {
		req.command = SET_FEATURES;
		req.timeout = 1000;

		if (strcmp(argv[0], "disable") == 0)
			req.features = WDSF_APM_DS;
		else if (strcmp(argv[0], "set") == 0 && argc >= 2 &&
		         (l = strtol(argv[1], NULL, 0)) >= 0 && l <= 253) {

			req.features = WDSF_APM_EN;
			req.sec_count = l + 1;
		} else
			usage();
	} else
		usage();

	ata_command(&req);
}


/*
 * Set the idle timer on the disk.  Set it for either idle mode or
 * standby mode, depending on how we were invoked.
 */

static void
device_setidle(int argc, char *argv[])
{
	unsigned long idle;
	struct atareq req;
	char *end;

	/* Only one argument */
	if (argc != 1)
		usage();

	idle = strtoul(argv[0], &end, 0);

	if (*end != '\0') {
		fprintf(stderr, "Invalid idle time: \"%s\"\n", argv[0]);
		exit(1);
	}

	if (idle > 19800) {
		fprintf(stderr, "Idle time has a maximum value of 5.5 "
			"hours\n");
		exit(1);
	}

	if (idle != 0 && idle < 5) {
		fprintf(stderr, "Idle timer must be at least 5 seconds\n");
		exit(1);
	}

	memset(&req, 0, sizeof(req));

	if (idle <= 240*5)
		req.sec_count = idle / 5;
	else
		req.sec_count = idle / (30*60) + 240;

	req.command = cmdname[3] == 's' ? WDCC_STANDBY : WDCC_IDLE;
	req.timeout = 1000;

	ata_command(&req);

	return;
}

/*
 * Query the device for the current power mode
 */

static void
device_checkpower(int argc, char *argv[])
{
	struct atareq req;

	/* No arguments. */
	if (argc != 0)
		usage();

	memset(&req, 0, sizeof(req));

	req.command = WDCC_CHECK_PWR;
	req.timeout = 1000;
	req.flags = ATACMD_READREG;

	ata_command(&req);

	printf("Current power status: ");

	switch (req.sec_count) {
	case 0x00:
		printf("Standby mode\n");
		break;
	case 0x80:
		printf("Idle mode\n");
		break;
	case 0xff:
		printf("Active mode\n");
		break;
	default:
		printf("Unknown power code (%02x)\n", req.sec_count);
	}

	return;
}

/*
 * device_smart:
 *
 *	Display SMART status
 */
static void
device_smart(int argc, char *argv[])
{
	struct atareq req;
	unsigned char inbuf[DEV_BSIZE];
	unsigned char inbuf2[DEV_BSIZE];

	if (argc < 1)
		usage();

	if (strcmp(argv[0], "enable") == 0) {
		memset(&req, 0, sizeof(req));

		req.features = WDSM_ENABLE_OPS;
		req.command = WDCC_SMART;
		req.cylinder = WDSMART_CYL;
		req.timeout = 1000;

		ata_command(&req);

		is_smart();
	} else if (strcmp(argv[0], "disable") == 0) {
		memset(&req, 0, sizeof(req));

		req.features = WDSM_DISABLE_OPS;
		req.command = WDCC_SMART;
		req.cylinder = WDSMART_CYL;
		req.timeout = 1000;

		ata_command(&req);

		is_smart();
	} else if (strcmp(argv[0], "status") == 0) {
		int rv;

		rv = is_smart();

		if (!rv) {
			fprintf(stderr, "SMART not supported\n");
			return;
		} else if (rv == 3)
			return;

		memset(&inbuf, 0, sizeof(inbuf));
		memset(&req, 0, sizeof(req));

		req.features = WDSM_STATUS;
		req.command = WDCC_SMART;
		req.cylinder = WDSMART_CYL;
		req.timeout = 1000;

		ata_command(&req);

		if (req.cylinder != WDSMART_CYL) {
			fprintf(stderr, "Threshold exceeds condition\n");
		}

		/* WDSM_RD_DATA and WDSM_RD_THRESHOLDS are optional
		 * features, the following ata_command()'s may error
		 * and exit().
		 */

		memset(&inbuf, 0, sizeof(inbuf));
		memset(&req, 0, sizeof(req));

		req.flags = ATACMD_READ;
		req.features = WDSM_RD_DATA;
		req.command = WDCC_SMART;
		req.databuf = (caddr_t) inbuf;
		req.datalen = sizeof(inbuf);
		req.cylinder = WDSMART_CYL;
		req.timeout = 1000;

		ata_command(&req);

		memset(&inbuf2, 0, sizeof(inbuf2));
		memset(&req, 0, sizeof(req));

		req.flags = ATACMD_READ;
		req.features = WDSM_RD_THRESHOLDS;
		req.command = WDCC_SMART;
		req.databuf = (caddr_t) inbuf2;
		req.datalen = sizeof(inbuf2);
		req.cylinder = WDSMART_CYL;
		req.timeout = 1000;

		ata_command(&req);

		print_smart_status(inbuf, inbuf2);

	} else if (strcmp(argv[0], "offline") == 0) {
		if (argc != 2)
			usage();
		if (!is_smart()) {
			fprintf(stderr, "SMART not supported\n");
			return;
		}

		memset(&req, 0, sizeof(req));

		req.features = WDSM_EXEC_OFFL_IMM;
		req.command = WDCC_SMART;
		req.cylinder = WDSMART_CYL;
		req.sec_num = atol(argv[1]);
		req.timeout = 10000;

		ata_command(&req);
	} else if (strcmp(argv[0], "error-log") == 0) {
		if (!is_smart()) {
			fprintf(stderr, "SMART not supported\n");
			return;
		}

		memset(&inbuf, 0, sizeof(inbuf));
		memset(&req, 0, sizeof(req));

		req.flags = ATACMD_READ;
		req.features = WDSM_RD_LOG;
		req.sec_count = 1;
		req.sec_num = 1;
		req.command = WDCC_SMART;
		req.databuf = (caddr_t) inbuf;
		req.datalen = sizeof(inbuf);
		req.cylinder = WDSMART_CYL;
		req.timeout = 1000;

		ata_command(&req);

		print_error(inbuf);
	} else if (strcmp(argv[0], "selftest-log") == 0) {
		if (!is_smart()) {
			fprintf(stderr, "SMART not supported\n");
			return;
		}

		memset(&inbuf, 0, sizeof(inbuf));
		memset(&req, 0, sizeof(req));

		req.flags = ATACMD_READ;
		req.features = WDSM_RD_LOG;
		req.sec_count = 1;
		req.sec_num = 6;
		req.command = WDCC_SMART;
		req.databuf = (caddr_t) inbuf;
		req.datalen = sizeof(inbuf);
		req.cylinder = WDSMART_CYL;
		req.timeout = 1000;

		ata_command(&req);

		print_selftest(inbuf);

	} else {
		usage();
	}
	return;
}

static void
device_security(int argc, char *argv[])
{
	struct atareq req;
	const struct ataparams *inqbuf;
	unsigned char data[DEV_BSIZE];
	char *pass;

	/* need subcommand */
	if (argc < 1)
		usage();

	memset(&req, 0, sizeof(req));
	if (strcmp(argv[0], "status") == 0) {
		inqbuf = getataparams();
		print_bitinfo("\t", "\n", inqbuf->atap_sec_st, ata_sec_st);
	} else if (strcmp(argv[0], "freeze") == 0) {
		req.command = WDCC_SECURITY_FREEZE;
		req.timeout = 1000;
		ata_command(&req);
	} else if ((strcmp(argv[0], "setpass") == 0) ||
	    (strcmp(argv[0], "unlock") == 0) ||
	    (strcmp(argv[0], "disable") == 0) ||
	    (strcmp(argv[0], "erase") == 0)) {
		if (argc != 2)
			usage();
		if (strcmp(argv[1], "user") != 0) {
			if (strcmp(argv[1], "master") == 0) {
				fprintf(stderr,
				    "Master passwords not supported\n");
				exit(1);
			} else {
				usage();
			}
		}

		pass = getpass("Password:");
		if (strlen(pass) > 32) {
			fprintf(stderr, "Password must be <=32 characters\n");
			exit(1);
		}

		req.flags |= ATACMD_WRITE;
		req.timeout = 1000;
		req.databuf = data;
		req.datalen = sizeof(data);
		memset(data, 0, sizeof(data));
		strlcpy((void *)&data[2], pass, 32 + 1);

		if (strcmp(argv[0], "setpass") == 0) {
			char orig[32 + 1];
			strlcpy(orig, pass, 32 + 1);
			pass = getpass("Confirm password:");
			if (0 != strcmp(orig, pass)) {
				fprintf(stderr, "Passwords do not match\n");
				exit(1);
			}
			req.command = WDCC_SECURITY_SET_PASSWORD;
		} else if (strcmp(argv[0], "unlock") == 0) {
			req.command = WDCC_SECURITY_UNLOCK;
		} else if (strcmp(argv[0], "disable") == 0) {
			req.command = WDCC_SECURITY_DISABLE_PASSWORD;
		} else if (strcmp(argv[0], "erase") == 0) {
			struct atareq prepare;

			inqbuf = getataparams();

			/*
			 * XXX Any way to lock the device to make sure
			 * this really is the command preceding the
			 * SECURITY ERASE UNIT command?  This would
			 * probably have to be moved into the kernel to
			 * do that.
			 */
			memset(&prepare, 0, sizeof(prepare));
			prepare.command = WDCC_SECURITY_ERASE_PREPARE;
			prepare.timeout = 1000;
			ata_command(&prepare);

			req.command = WDCC_SECURITY_ERASE_UNIT;

			/*
			 * Enable enhanced erase if it's supported.
			 *
			 * XXX should be a command-line option
			 */
			if (inqbuf->atap_sec_st & WDC_SEC_ESE_SUPP) {
				data[0] |= 0x2;
				req.timeout = (inqbuf->atap_eseu_time & 0xff)
				    * 2 * 60 * 1000;
			} else {
				req.timeout = (inqbuf->atap_seu_time & 0xff)
				    * 2 * 60 * 1000;
			}

			/*
			 * If the estimated time was 0xff (* 2 * 60 *
			 * 1000 = 30600000), that means `>508 minutes'.
			 * Estimate that we can handle 16 MB/sec, a
			 * rate I just pulled out of my arse.
			 */
			if (req.timeout == 30600000) {
				uint64_t bytes, timeout;
				compute_capacity(inqbuf, &bytes, NULL, NULL);
				timeout = (bytes / (16 * 1024 * 1024)) * 1000;
				if (timeout > (uint64_t)INT_MAX)
					req.timeout = INT_MAX;
				else
					req.timeout = timeout;
			}

			printf("Erasing may take up to %dh %dm %ds...\n",
			    (req.timeout / 1000 / 60) / 60,
			    (req.timeout / 1000 / 60) % 60,
			    req.timeout % 60);
		} else {
			abort();
		}

		ata_command(&req);
	} else {
		usage();
	}
}

/*
 * bus_reset:
 *	Reset an ATA bus (will reset all devices on the bus)
 */
static void
bus_reset(int argc, char *argv[])
{
	int error;

	/* no args */
	if (argc != 0)
		usage();

	error = ioctl(fd, ATABUSIORESET, NULL);

	if (error == -1)
		err(1, "ATABUSIORESET failed");
}
