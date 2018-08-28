/*	$NetBSD: amrctl.c,v 1.11 2018/08/27 00:36:03 sevan Exp $	*/

/*-
 * Copyright (c) 2002, Pierre David <Pierre.David@crc.u-strasbg.fr>
 * Copyright (c) 2006, Jung-uk Kim <jkim@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: amrctl.c,v 1.11 2018/08/27 00:36:03 sevan Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <machine/param.h>

#include <dev/pci/amrio.h>
#include <dev/pci/amrreg.h>

#define NATTEMPTS	5
#define SLEEPTIME	100000	/* microseconds */

static int	nattempts = NATTEMPTS;	/* # of attempts before giving up */
static int	sleeptime = SLEEPTIME;	/* between attempts, in ms */

#define AMR_BUFSIZE	1024

static int	enq_result = AMR_STATUS_FAILED;
static char	enq_buffer[AMR_BUFSIZE];

#define AMR_MAX_NCTRLS	16
#define AMR_MAX_NSDEVS	16

static uint8_t	nschan = 0;

/*
 * Include lookup tables, and a function to match a code to a string.
 *
 * XXX Lookup tables cannot be included, since they require symbols from
 * amrreg.h which need in turn the _KERNEL define.
 */

/* #define AMR_DEFINE_TABLES */
/* #include "amr_tables.h" */

static int amr_ioctl_enquiry(int, uint8_t, uint8_t, uint8_t);
__dead static void usage(const char *);
static int describe_card(int, int, int);
static char * describe_property(uint8_t, char *);
static const char * describe_state(int, uint8_t);
static void describe_battery(int, int, int, int, int);
static void describe_one_volume(int, int, uint32_t, uint8_t, uint8_t);
static void describe_one_drive(int, int, uint8_t);
static void describe_drive(int, int, int, int, int);

/*
 * Offsets in an amr_user_ioctl.au_cmd [] array See amrio.h
 */

#define MB_COMMAND	0
#define MB_CHANNEL	1
#define MB_PARAM	2
#define MB_PAD		3
#define MB_DRIVE	4

#define FIRMWARE_40LD	1
#define FIRMWARE_8LD	2

static const struct {
	const char	*product;
	const uint32_t	signature;
} prodtable[] = {
	{	"Series 431",			AMR_SIG_431	},
	{	"Series 438",			AMR_SIG_438	},
	{	"Series 762",			AMR_SIG_762	},
	{	"Integrated HP NetRAID (T5)",	AMR_SIG_T5	},
	{	"Series 466",			AMR_SIG_466	},
	{	"Series 467",			AMR_SIG_467	},
	{	"Integrated HP NetRAID (T7)",	AMR_SIG_T7	},
	{	"Series 490",			AMR_SIG_490	}
};

static const struct {
	const int	code;
	const char	*ifyes, *ifno;
} proptable[] = {
	{	AMR_DRV_WRITEBACK,
		"writeback",		"write-through"		},
	{	AMR_DRV_READHEAD,
		"read-ahead",		"no-read-ahead"		},
	{	AMR_DRV_ADAPTIVE,
		"adaptative-io",	"no-adaptative-io"	}
};

static const struct {
	const int	code;
	const char	*status;
} statetable[] = {
	{	AMR_DRV_OFFLINE,	"offline"	},
	{	AMR_DRV_DEGRADED,	"degraded"	},
	{	AMR_DRV_OPTIMAL,	"optimal"	},
	{	AMR_DRV_ONLINE,		"online"	},
	{	AMR_DRV_FAILED,		"failed"	},
	{	AMR_DRV_REBUILD,	"rebuild"	},
	{	AMR_DRV_HOTSPARE,	"hotspare"	}
};

static const struct {
	const uint8_t	code;
	const char		*status;
} battable[] = {
	{	AMR_BATT_MODULE_MISSING,	"not present"		},
	{	AMR_BATT_LOW_VOLTAGE,		"low voltage"		},
	{	AMR_BATT_TEMP_HIGH,		"high temperature"	},
	{	AMR_BATT_PACK_MISSING,		"pack missing"	},
	{	AMR_BATT_CYCLES_EXCEEDED,	"cycle exceeded"	}
};

static const struct {
	const uint8_t	code;
	const char		*status;
} bcstatble[] = {
	{	AMR_BATT_CHARGE_DONE,		"charge done"		},
	{	AMR_BATT_CHARGE_INPROG,		"charge in progress"	},
	{	AMR_BATT_CHARGE_FAIL,		"charge failed"		}
};

static int
amr_ioctl_enquiry(int fd, uint8_t cmd, uint8_t cmdsub, uint8_t cmdqual)
{
	struct amr_user_ioctl am;
	int	r, i;

	am.au_cmd[MB_COMMAND] = cmd;
	am.au_cmd[MB_CHANNEL] = cmdsub;
	am.au_cmd[MB_PARAM] = cmdqual;
	am.au_cmd[MB_PAD] = 0;
	am.au_cmd[MB_DRIVE] = 0;

	am.au_buffer = enq_buffer;
	am.au_length = AMR_BUFSIZE;
	am.au_direction = AMR_IO_READ;
	am.au_status = 0;

	i = 0;
	r = -1;
	while (i < nattempts && r == -1) {
		r = ioctl(fd, AMR_IO_COMMAND, &am);
		if (r == -1) {
			if (errno != EBUSY) {
				warn("ioctl enquiry");
				return -1;
			} else
				usleep(sleeptime);
		}
		i++;
	}
	return am.au_status;
}

static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s stat [-a num] [-b] "
		"[-f dev] [-g] [-l vol]\n\t\t"
		"[-p drive|-s bus[:target]] [-t usec] [-v]\n\n\t"
		"-a num\t\tnumber of retries\n\t"
		"-b\t\tbattery status\n\t"
		"-f dev\t\tdevice path\n\t"
		"-g\t\tprint global parameters\n\t"
		"-l vol\t\tlogical volume ID\n\t"
		"-p drive\tphysical drive ID\n\t"
		"-s bus[:target]\tSCSI bus (and optinal target)\n\t"
		"-t usec\t\tsleep time between retries\n\t"
		"-v\t\tverbose output\n",
		prog);
	exit(1);
}

/******************************************************************************
 * Card description
 */

static int
describe_card(int fd, int verbosity, int globalparam)
{
	struct amr_enquiry *ae;
	uint32_t	cardtype;

	/*
	 * Try the 40LD firmware interface
	 */

	enq_result = amr_ioctl_enquiry(fd, AMR_CMD_CONFIG,
		AMR_CONFIG_PRODUCT_INFO, 0);
	if (enq_result == AMR_STATUS_SUCCESS) {
		struct amr_prodinfo *ap;

		ap = (struct amr_prodinfo *)enq_buffer;
		nschan = ap->ap_nschan;
		if (globalparam) {
			printf("Product\t\t\t<%.80s>\n", ap->ap_product);
			printf("Firmware\t\t%.16s\n", ap->ap_firmware);
			printf("BIOS\t\t\t%.16s\n", ap->ap_bios);
			printf("SCSI channels\t\t%d\n", ap->ap_nschan);
			printf("Fibre loops\t\t%d\n", ap->ap_fcloops);
			printf("Memory size\t\t%d MB\n", ap->ap_memsize);
			if (verbosity >= 1) {
				printf("Ioctl\t\t\t%d (%s)\n", FIRMWARE_40LD,
				       "40LD");
				printf("Signature\t\t0x%08x\n",
				       ap->ap_signature);
				printf("Configsig\t\t0x%08x\n",
				       ap->ap_configsig);
				printf("Subsystem\t\t0x%04x\n",
				       ap->ap_subsystem);
				printf("Subvendor\t\t0x%04x\n",
				       ap->ap_subvendor);
				printf("Notify counters\t\t%d\n",
				       ap->ap_numnotifyctr);
			}
		}
		return FIRMWARE_40LD;
	}
	/*
	 * Try the 8LD firmware interface
	 */

	enq_result = amr_ioctl_enquiry(fd, AMR_CMD_EXT_ENQUIRY2, 0, 0);
	ae = (struct amr_enquiry *)enq_buffer;
	if (enq_result == AMR_STATUS_SUCCESS) {
		cardtype = ae->ae_signature;
	} else {
		enq_result = amr_ioctl_enquiry(fd, AMR_CMD_ENQUIRY, 0, 0);
		cardtype = 0;
	}

	if (enq_result == AMR_STATUS_SUCCESS) {

		if (globalparam) {
			const char   *product = NULL;
			char	bios[100], firmware[100];
			size_t	i;

			for (i = 0; i < __arraycount(prodtable); i++) {
				if (cardtype == prodtable[i].signature) {
					product = prodtable[i].product;
					break;
				}
			}
			if (product == NULL)
				product = "unknown card signature";

			/*
			 * HP NetRaid controllers have a special encoding of
			 * the firmware and BIOS versions. The AMI version
			 * seems to have it as strings whereas the HP version
			 * does it with a leading uppercase character and two
			 * binary numbers.
			 */

			if (ae->ae_adapter.aa_firmware[2] >= 'A' &&
			    ae->ae_adapter.aa_firmware[2] <= 'Z' &&
			    ae->ae_adapter.aa_firmware[1] < ' ' &&
			    ae->ae_adapter.aa_firmware[0] < ' ' &&
			    ae->ae_adapter.aa_bios[2] >= 'A' &&
			    ae->ae_adapter.aa_bios[2] <= 'Z' &&
			    ae->ae_adapter.aa_bios[1] < ' ' &&
			    ae->ae_adapter.aa_bios[0] < ' ') {

				/*
				 * looks like we have an HP NetRaid version
				 * of the MegaRaid
				 */

				if (cardtype == AMR_SIG_438) {
					/*
					 * the AMI 438 is a NetRaid 3si in
					 * HP-land
					 */
					product = "HP NetRaid 3si";
				}
				snprintf(firmware, sizeof(firmware),
					"%c.%02d.%02d",
					ae->ae_adapter.aa_firmware[2],
					ae->ae_adapter.aa_firmware[1],
					ae->ae_adapter.aa_firmware[0]);
				snprintf(bios, sizeof(bios),
					"%c.%02d.%02d",
					ae->ae_adapter.aa_bios[2],
					ae->ae_adapter.aa_bios[1],
					ae->ae_adapter.aa_bios[0]);
			} else {
				snprintf(firmware, sizeof(firmware), "%.4s",
					ae->ae_adapter.aa_firmware);
				snprintf(bios, sizeof(bios), "%.4s",
					ae->ae_adapter.aa_bios);
			}

			printf("Ioctl = %d (%s)\n", FIRMWARE_8LD, "8LD");
			printf("Product =\t<%s>\n", product);
			printf("Firmware =\t%s\n", firmware);
			printf("BIOS =\t%s\n", bios);
			/* printf ("SCSI Channels =\t%d\n", ae->ae_nschan); */
			/* printf ("Fibre Loops =\t%d\n", ae->ae_fcloops); */
			printf("Memory size =\t%d MB\n",
			       ae->ae_adapter.aa_memorysize);
			/*
			 * printf ("Notify counters =\t%d\n",
			 * ae->ae_numnotifyctr) ;
			 */
		}
		return FIRMWARE_8LD;
	}
	/*
	 * Neither firmware interface succeeded. Abort.
	 */

	fprintf(stderr, "Firmware interface not supported\n");
	exit(1);

}

static char *
describe_property(uint8_t prop, char *buffer)
{
	size_t	i;

	strcpy(buffer, "<");
	for (i = 0; i < __arraycount(proptable); i++) {
		if (i > 0)
			strcat(buffer, ",");
		if (prop & proptable[i].code)
			strcat(buffer, proptable[i].ifyes);
		else
			strcat(buffer, proptable[i].ifno);
	}
	strcat(buffer, ">");

	return buffer;
}

static const char *
describe_state(int verbosity, uint8_t state)
{
	size_t	i;

	if ((AMR_DRV_PREVSTATE(state) == AMR_DRV_CURSTATE(state)) &&
	    (AMR_DRV_CURSTATE(state) == AMR_DRV_OFFLINE) && verbosity == 0)
		return NULL;

	for (i = 0; i < __arraycount(statetable); i++)
		if (AMR_DRV_CURSTATE(state) == statetable[i].code)
			return (statetable[i].status);

	return NULL;
}

/******************************************************************************
 * Battery status
 */
static void
describe_battery(int fd, int verbosity, int fwint, int bflags, int globalparam)
{
	uint8_t batt_status;
	size_t i;

	if (fwint == FIRMWARE_40LD) {
		enq_result = amr_ioctl_enquiry(fd, AMR_CMD_CONFIG,
			AMR_CONFIG_ENQ3, AMR_CONFIG_ENQ3_SOLICITED_FULL);
		if (enq_result == AMR_STATUS_SUCCESS) {
			struct amr_enquiry3 *ae3;

			ae3 = (struct amr_enquiry3 *)enq_buffer;
			if (bflags || globalparam) {
				batt_status = ae3->ae_batterystatus;
				printf("Battery status\t\t");
				for (i = 0; i < __arraycount(battable); i++) {
					if (batt_status & battable[i].code)
						printf("%s, ", battable[i].status);
				}
				if (!(batt_status &
				    (AMR_BATT_MODULE_MISSING|AMR_BATT_PACK_MISSING))) {
					for (i = 0;
					     i < __arraycount(bcstatble); i++)
						if (bcstatble[i].code ==
						    (batt_status & AMR_BATT_CHARGE_MASK))
							printf("%s", bcstatble[i].status);
				} else
					printf("charge unknown");
				if (verbosity)
					printf(" (0x%02x)", batt_status);
				printf("\n");
			}
		}
	} else if (fwint == FIRMWARE_8LD) {
		/* Nothing to do here. */
		return;
	} else {
		fprintf(stderr, "Firmware interface not supported.\n");
		exit(1);
	}

	return;
}

/******************************************************************************
 * Logical volumes
 */

static void
describe_one_volume(int ldrv, int verbosity,
		    uint32_t size, uint8_t state, uint8_t prop)
{
	float	szgb;
	int	raid_level;
	char	propstr[MAXPATHLEN];
	const char *statestr;

	szgb = ((float)size) / (1024 * 1024 * 2);	/* size in GB */

	raid_level = prop & AMR_DRV_RAID_MASK;

	printf("Logical volume %d\t", ldrv);
	statestr = describe_state(verbosity, state);
	printf("%s ", statestr);
	printf("(%.2f GB, RAID%d", szgb, raid_level);
	if (verbosity >= 1) {
		describe_property(prop, propstr);
		printf(" %s", propstr);
	}
	printf(")\n");
}

/******************************************************************************
 * Physical drives
 */

static void
describe_one_drive(int pdrv, int verbosity, uint8_t state)
{
	const char *statestr;

	statestr = describe_state(verbosity, state);
	if (statestr) {
		if (nschan > 0)
			printf("Physical drive %d:%d\t%s\n",
			       pdrv / AMR_MAX_NSDEVS, pdrv % AMR_MAX_NSDEVS,
			       statestr);
		else
			printf("Physical drive %d:\t%s\n", pdrv, statestr);
	}
}

static void
describe_drive(int verbosity, int fwint, int ldrv, int sbus, int sdev)
{
	int	drv, pdrv = -1;

	if (sbus > -1 && sdev > -1)
		pdrv = (sbus * AMR_MAX_NSDEVS) + sdev;
	if (nschan != 0) {
		if (sbus > -1 && sbus >= nschan) {
			fprintf(stderr, "SCSI channel %d does not exist.\n", sbus);
			exit(1);
		} else if (sdev > -1 && sdev >= AMR_MAX_NSDEVS) {
			fprintf(stderr, "SCSI device %d:%d does not exist.\n",
				sbus, sdev);
			exit(1);
		}
	}
	if (fwint == FIRMWARE_40LD) {
		if (enq_result == AMR_STATUS_SUCCESS) {
			struct amr_enquiry3 *ae3;

			ae3 = (struct amr_enquiry3 *)enq_buffer;
			if ((ldrv < 0 && sbus < 0) || ldrv >= 0) {
				if (ldrv >= ae3->ae_numldrives) {
					fprintf(stderr, "Logical volume %d "
						"does not exist.\n", ldrv);
					exit(1);
				}
				if (ldrv < 0) {
					for (drv = 0;
					     drv < ae3->ae_numldrives;
					     drv++)
						describe_one_volume(drv,
						    verbosity,
						    ae3->ae_drivesize[drv],
						    ae3->ae_drivestate[drv],
						    ae3->ae_driveprop[drv]);
				} else {
					describe_one_volume(ldrv,
					    verbosity,
					    ae3->ae_drivesize[ldrv],
					    ae3->ae_drivestate[ldrv],
					    ae3->ae_driveprop[ldrv]);
				}
			}
			if ((ldrv < 0 && sbus < 0) || sbus >= 0) {
				if (pdrv >= AMR_40LD_MAXPHYSDRIVES ||
				    (nschan != 0 && pdrv >= (nschan * AMR_MAX_NSDEVS))) {
					fprintf(stderr, "Physical drive %d "
						"is out of range.\n", pdrv);
					exit(1);
				}
				if (sbus < 0) {
					for (drv = 0;
					     drv < AMR_40LD_MAXPHYSDRIVES;
					     drv++) {
						if (nschan != 0 &&
						    drv >= (nschan * AMR_MAX_NSDEVS))
							break;
						describe_one_drive(drv,
						    verbosity,
						    ae3->ae_pdrivestate[drv]);
					}
				} else if (sdev < 0) {
					for (drv = sbus * AMR_MAX_NSDEVS;
					     drv < ((sbus + 1) * AMR_MAX_NSDEVS);
					     drv++) {
						if (nschan != 0 &&
						    drv >= (nschan * AMR_MAX_NSDEVS))
							break;
						describe_one_drive(drv,
						    verbosity,
						    ae3->ae_pdrivestate[drv]);
					}
				} else {
					if (nschan != 0 &&
					    pdrv < (nschan * AMR_MAX_NSDEVS))
						describe_one_drive(pdrv, 1,
						    ae3->ae_pdrivestate[pdrv]);
				}
			}
		}
	} else if (fwint == FIRMWARE_8LD) {
		/* Nothing to do here. */
		return;
	} else {
		fprintf(stderr, "Firmware interface not supported.\n");
		exit(1);
	}
}

/******************************************************************************
 * Main function
 */

int
main(int argc, char *argv[])
{
	int	i;
	int	fd = -1;
	int	globalparam = 0, verbosity = 0;
	int	bflags = 0, fflags = 0, sflags = 0;
	int	lvolno = -1, physno = -1;
	int	sbusno = -1, targetno = -1;
	char	filename[MAXPATHLEN];
	char	sdev[MAXPATHLEN];
	char	*pdev;

	extern char *optarg;
	extern int optind;

	/*
	 * Parse arguments
	 */
	if (argc < 2)
		usage(argv[0]);
	if (strcmp(argv[1], "stat") != 0) /* only stat implemented for now */
		usage(argv[0]);

	optind = 2;
	while ((i = getopt(argc, argv, "a:b:f:gl:p:s:t:v")) != -1)
		switch (i) {
		case 'a':
			nattempts = atoi(optarg);
			break;
		case 'b':
			bflags++;
			break;
		case 'f':
			snprintf(filename, MAXPATHLEN, "%s", optarg);
			filename[MAXPATHLEN - 1] = '\0';
			fflags++;
			break;
		case 'g':
			globalparam = 1;
			break;
		case 'l':
			lvolno = atoi(optarg);
			break;
		case 'p':
			physno = atoi(optarg);
			break;
		case 's':
			snprintf(sdev, MAXPATHLEN, "%s", optarg);
			sdev[MAXPATHLEN - 1] = '\0';
			sflags++;
			break;
		case 't':
			sleeptime = atoi(optarg);
			break;
		case 'v':
			verbosity++;
			break;
		case '?':
		default:
			usage(argv[0]);
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage(argv[0]);

	if (!fflags) {
		snprintf(filename, MAXPATHLEN, "/dev/amr0");
	}
		
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "open");
	}
	if (ioctl(fd, AMR_IO_VERSION, &i) == -1) {
		err(EXIT_FAILURE, "ioctl version");
	}

	if (sflags) {
		if(physno > -1)
			usage(argv[0]);
		else {
			sbusno = atoi(sdev);
			if ((pdev = index(sdev, ':')))
				targetno = atoi(++pdev);
		}
	} else if (physno > -1) {
		sbusno = physno / AMR_MAX_NSDEVS;
		targetno = physno % AMR_MAX_NSDEVS;
	}
	
	if (globalparam && verbosity >= 1)
		printf("Version\t\t\t%d\n", i);
#if 0
	if (i != 1) {
		fprintf(stderr, "Driver version (%d) not supported\n", i);
		exit(1);
	}
#endif

	i = describe_card(fd, verbosity, globalparam);
	describe_battery(fd, verbosity, i, bflags, globalparam);
	if (!bflags || lvolno > -1 || physno > -1 || sbusno > -1 || targetno > -1)
		describe_drive(verbosity, i, lvolno, sbusno, targetno);

	return 0;
}
