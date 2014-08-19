/*      $NetBSD: raidctl.c,v 1.55.8.1 2014/08/20 00:02:27 tls Exp $   */

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Greg Oster
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
 * This program is a re-write of the original rf_ctrl program 
 * distributed by CMU with RAIDframe 1.1.
 *
 * This program is the user-land interface to the RAIDframe kernel
 * driver in NetBSD.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: raidctl.c,v 1.55.8.1 2014/08/20 00:02:27 tls Exp $");
#endif


#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/disklabel.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/raidframe/raidframevar.h>
#include <dev/raidframe/raidframeio.h>
#include "rf_configure.h"
#include "prog_ops.h"

void	do_ioctl(int, u_long, void *, const char *);
static  void rf_configure(int, char*, int);
static  const char *device_status(RF_DiskStatus_t);
static  void rf_get_device_status(int);
static	void rf_output_configuration(int, const char *);
static  void get_component_number(int, char *, int *, int *);
static  void rf_fail_disk(int, char *, int);
__dead static  void usage(void);
static  void get_component_label(int, char *);
static  void set_component_label(int, char *);
static  void init_component_labels(int, int);
static  void set_autoconfig(int, int, char *);
static  void add_hot_spare(int, char *);
static  void remove_hot_spare(int, char *);
static  void rebuild_in_place(int, char *);
static  void check_status(int,int);
static  void check_parity(int,int, char *);
static  void do_meter(int, u_long);
static  void get_bar(char *, double, int);
static  void get_time_string(char *, int);
static  void rf_output_pmstat(int, int);
static  void rf_pm_configure(int, int, char *, int[]);

int verbose;

static const char *rootpart[] = { "No", "Force", "Soft", "*invalid*" };

int
main(int argc,char *argv[])
{
	int ch, i;
	int num_options;
	unsigned long action;
	char config_filename[PATH_MAX];
	char dev_name[PATH_MAX];
	char name[PATH_MAX];
	char component[PATH_MAX];
	char autoconf[10];
	char *parityconf = NULL;
	int parityparams[3];
	int do_output;
	int do_recon;
	int do_rewrite;
	int raidID;
	int serial_number;
	struct stat st;
	int fd;
	int force;
	int openmode;

	num_options = 0;
	action = 0;
	do_output = 0;
	do_recon = 0;
	do_rewrite = 0;
	serial_number = 0;
	force = 0;
	openmode = O_RDWR;	/* default to read/write */

	while ((ch = getopt(argc, argv, "a:A:Bc:C:f:F:g:GiI:l:mM:r:R:sSpPuv")) 
	       != -1)
		switch(ch) {
		case 'a':
			action = RAIDFRAME_ADD_HOT_SPARE;
			strlcpy(component, optarg, sizeof(component));
			num_options++;
			break;
		case 'A':
			action = RAIDFRAME_SET_AUTOCONFIG;
			strlcpy(autoconf, optarg, sizeof(autoconf));
			num_options++;
			break;
		case 'B':
			action = RAIDFRAME_COPYBACK;
			num_options++;
			break;
		case 'c':
			action = RAIDFRAME_CONFIGURE;
			strlcpy(config_filename, optarg,
			    sizeof(config_filename));
			force = 0;
			num_options++;
			break;
		case 'C':
			strlcpy(config_filename, optarg,
			    sizeof(config_filename));
			action = RAIDFRAME_CONFIGURE;
			force = 1;
			num_options++;
			break;
		case 'f':
			action = RAIDFRAME_FAIL_DISK;
			strlcpy(component, optarg, sizeof(component));
			do_recon = 0;
			num_options++;
			break;
		case 'F':
			action = RAIDFRAME_FAIL_DISK;
			strlcpy(component, optarg, sizeof(component));
			do_recon = 1;
			num_options++;
			break;
		case 'g':
			action = RAIDFRAME_GET_COMPONENT_LABEL;
			strlcpy(component, optarg, sizeof(component));
			openmode = O_RDONLY;
			num_options++;
			break;
		case 'G':
			action = RAIDFRAME_GET_INFO;
			openmode = O_RDONLY;
			do_output = 1;
			num_options++;
			break;
		case 'i':
			action = RAIDFRAME_REWRITEPARITY;
			num_options++;
			break;
		case 'I':
			action = RAIDFRAME_INIT_LABELS;
			serial_number = atoi(optarg);
			num_options++;
			break;
		case 'm':
			action = RAIDFRAME_PARITYMAP_STATUS;
			openmode = O_RDONLY;
			num_options++;
			break;
		case 'M':
			action = RAIDFRAME_PARITYMAP_SET_DISABLE;
			parityconf = strdup(optarg);
			num_options++;
			/* XXXjld: should rf_pm_configure do the atoi()s? */
			i = 0;
			while (i < 3 && optind < argc &&
			    isdigit((int)argv[optind][0]))
				parityparams[i++] = atoi(argv[optind++]);
			while (i < 3)
				parityparams[i++] = 0;
			break;
		case 'l': 
			action = RAIDFRAME_SET_COMPONENT_LABEL;
			strlcpy(component, optarg, sizeof(component));
			num_options++;
			break;
		case 'r':
			action = RAIDFRAME_REMOVE_HOT_SPARE;
			strlcpy(component, optarg, sizeof(component));
			num_options++;
			break;
		case 'R':
			strlcpy(component, optarg, sizeof(component));
			action = RAIDFRAME_REBUILD_IN_PLACE;
			num_options++;
			break;
		case 's':
			action = RAIDFRAME_GET_INFO;
			openmode = O_RDONLY;
			num_options++;
			break;
		case 'S':
			action = RAIDFRAME_CHECK_RECON_STATUS_EXT;
			openmode = O_RDONLY;
			num_options++;
			break;
		case 'p':
			action = RAIDFRAME_CHECK_PARITY;
			openmode = O_RDONLY;
			num_options++;
			break;
		case 'P':
			action = RAIDFRAME_CHECK_PARITY;
			do_rewrite = 1;
			num_options++;
			break;
		case 'u':
			action = RAIDFRAME_SHUTDOWN;
			num_options++;
			break;
		case 'v':
			verbose = 1;
			/* Don't bump num_options, as '-v' is not 
			   an option like the others */
			/* num_options++; */
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if ((num_options > 1) || (argc == 0)) 
		usage();

	if (prog_init && prog_init() == -1)
		err(1, "init failed");

	strlcpy(name, argv[0], sizeof(name));
	fd = opendisk1(name, openmode, dev_name, sizeof(dev_name), 0,
	    prog_open);
	if (fd == -1)
		err(1, "Unable to open device file: %s", name);
	if (prog_fstat(fd, &st) == -1)
		err(1, "stat failure on: %s", dev_name);
	if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
		err(1, "invalid device: %s", dev_name);

	raidID = DISKUNIT(st.st_rdev);

	switch(action) {
	case RAIDFRAME_ADD_HOT_SPARE:
		add_hot_spare(fd, component);
		break;
	case RAIDFRAME_REMOVE_HOT_SPARE:
		remove_hot_spare(fd, component);
		break;
	case RAIDFRAME_CONFIGURE:
		rf_configure(fd, config_filename, force);
		break;
	case RAIDFRAME_SET_AUTOCONFIG:
		set_autoconfig(fd, raidID, autoconf);
		break;
	case RAIDFRAME_COPYBACK:
		printf("Copyback.\n");
		do_ioctl(fd, RAIDFRAME_COPYBACK, NULL, "RAIDFRAME_COPYBACK");
		if (verbose) {
			sleep(3); /* XXX give the copyback a chance to start */
			printf("Copyback status:\n");
			do_meter(fd,RAIDFRAME_CHECK_COPYBACK_STATUS_EXT);
		}
		break;
	case RAIDFRAME_FAIL_DISK:
		rf_fail_disk(fd, component, do_recon);
		break;
	case RAIDFRAME_SET_COMPONENT_LABEL:
		set_component_label(fd, component);
		break;
	case RAIDFRAME_GET_COMPONENT_LABEL:
		get_component_label(fd, component);
		break;
	case RAIDFRAME_INIT_LABELS:
		init_component_labels(fd, serial_number);
		break;
	case RAIDFRAME_REWRITEPARITY:
		printf("Initiating re-write of parity\n");
		do_ioctl(fd, RAIDFRAME_REWRITEPARITY, NULL, 
			 "RAIDFRAME_REWRITEPARITY");
		if (verbose) {
			sleep(3); /* XXX give it time to get started */
			printf("Parity Re-write status:\n");
			do_meter(fd, RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT);
		}
		break;
	case RAIDFRAME_CHECK_RECON_STATUS_EXT:
		check_status(fd,1);
		break;
	case RAIDFRAME_GET_INFO:
		if (do_output)
			rf_output_configuration(fd, dev_name);
		else
			rf_get_device_status(fd);
		break;
	case RAIDFRAME_PARITYMAP_STATUS:
		rf_output_pmstat(fd, raidID);
		break;
	case RAIDFRAME_PARITYMAP_SET_DISABLE:
		rf_pm_configure(fd, raidID, parityconf, parityparams);
		break;
	case RAIDFRAME_REBUILD_IN_PLACE:
		rebuild_in_place(fd, component);
		break;
	case RAIDFRAME_CHECK_PARITY:
		check_parity(fd, do_rewrite, dev_name);
		break;
	case RAIDFRAME_SHUTDOWN:
		do_ioctl(fd, RAIDFRAME_SHUTDOWN, NULL, "RAIDFRAME_SHUTDOWN");
		break;
	default:
		break;
	}

	prog_close(fd);
	exit(0);
}

void
do_ioctl(int fd, unsigned long command, void *arg, const char *ioctl_name)
{
	if (prog_ioctl(fd, command, arg) == -1)
		err(1, "ioctl (%s) failed", ioctl_name);
}


static void
rf_configure(int fd, char *config_file, int force)
{
	void *generic;
	RF_Config_t cfg;

	if (rf_MakeConfig( config_file, &cfg ) != 0)
		err(1, "Unable to create RAIDframe configuration structure");
	
	cfg.force = force;

	/* 
	 * Note the extra level of redirection needed here, since
	 * what we really want to pass in is a pointer to the pointer to 
	 * the configuration structure. 
	 */

	generic = &cfg;
	do_ioctl(fd, RAIDFRAME_CONFIGURE, &generic, "RAIDFRAME_CONFIGURE");
}

static const char *
device_status(RF_DiskStatus_t status)
{

	switch (status) {
	case rf_ds_optimal:
		return ("optimal");
		break;
	case rf_ds_failed:
		return ("failed");
		break;
	case rf_ds_reconstructing:
		return ("reconstructing");
		break;
	case rf_ds_dist_spared:
		return ("dist_spared");
		break;
	case rf_ds_spared:
		return ("spared");
		break;
	case rf_ds_spare:
		return ("spare");
		break;
	case rf_ds_used_spare:
		return ("used_spare");
		break;
	default:
		return ("UNKNOWN");
	}
	/* NOTREACHED */
}

static void
rf_get_device_status(int fd)
{
	RF_DeviceConfig_t device_config;
	void *cfg_ptr;
	int is_clean;
	int i;

	cfg_ptr = &device_config;

	do_ioctl(fd, RAIDFRAME_GET_INFO, &cfg_ptr, "RAIDFRAME_GET_INFO");

	printf("Components:\n");
	for(i=0; i < device_config.ndevs; i++) {
		printf("%20s: %s\n", device_config.devs[i].devname, 
		       device_status(device_config.devs[i].status));
	}
	if (device_config.nspares > 0) {
		printf("Spares:\n");
		for(i=0; i < device_config.nspares; i++) {
			printf("%20s: %s\n",
			       device_config.spares[i].devname, 
			       device_status(device_config.spares[i].status));
		}
	} else {
		printf("No spares.\n");
	}
	for(i=0; i < device_config.ndevs; i++) {
		if (device_config.devs[i].status == rf_ds_optimal) {
			get_component_label(fd, device_config.devs[i].devname);
		} else {
			printf("%s status is: %s.  Skipping label.\n",
			       device_config.devs[i].devname,
			       device_status(device_config.devs[i].status));
		}
	}

	if (device_config.nspares > 0) {
		for(i=0; i < device_config.nspares; i++) {
			if ((device_config.spares[i].status == 
			     rf_ds_optimal) ||
			    (device_config.spares[i].status == 
			     rf_ds_used_spare)) {
				get_component_label(fd, 
					    device_config.spares[i].devname);
			} else {
				printf("%s status is: %s.  Skipping label.\n",
				       device_config.spares[i].devname,
				       device_status(device_config.spares[i].status));
			}		
		}
	}

	do_ioctl(fd, RAIDFRAME_CHECK_PARITY, &is_clean,
		 "RAIDFRAME_CHECK_PARITY");
	if (is_clean) {
		printf("Parity status: clean\n");
	} else {
		printf("Parity status: DIRTY\n");
	}
	check_status(fd,0);
}

static void
rf_output_pmstat(int fd, int raidID)
{
	char srs[7];
	unsigned int i, j;
	int dis, dr;
	struct rf_pmstat st;

	if (prog_ioctl(fd, RAIDFRAME_PARITYMAP_STATUS, &st) == -1) {
		if (errno == EINVAL) {
			printf("raid%d: has no parity; parity map disabled\n",
				raidID);
			return;
		}
		err(1, "ioctl (%s) failed", "RAIDFRAME_PARITYMAP_STATUS");
	}

	if (st.enabled) {
		if (0 > humanize_number(srs, 7, st.region_size * DEV_BSIZE, 
			"B", HN_AUTOSCALE, HN_NOSPACE))
			strlcpy(srs, "???", 7);

		printf("raid%d: parity map enabled with %u regions of %s\n",
		    raidID, st.params.regions, srs);
		printf("raid%d: regions marked clean after %d intervals of"
		    " %d.%03ds\n", raidID, st.params.cooldown,
		    st.params.tickms / 1000, st.params.tickms % 1000);
		printf("raid%d: write/sync/clean counters "
		    "%"PRIu64"/%"PRIu64"/%"PRIu64"\n", raidID,
		    st.ctrs.nwrite, st.ctrs.ncachesync, st.ctrs.nclearing);

		dr = 0;
		for (i = 0; i < st.params.regions; i++)
			if (isset(st.dirty, i))
				dr++;
		printf("raid%d: %d dirty region%s\n", raidID, dr,
		    dr == 1 ? "" : "s");

		if (verbose > 0) {
			for (i = 0; i < RF_PARITYMAP_NBYTE; i += 32) {
				printf("    ");
				for (j = i; j < RF_PARITYMAP_NBYTE
					 && j < i + 32; j++)
					printf("%x%x", st.dirty[j] & 15, 
					    (st.dirty[j] >> 4) & 15);
				printf("\n");
			}
		}
	} else {
		printf("raid%d: parity map disabled\n", raidID);
	}

	do_ioctl(fd, RAIDFRAME_PARITYMAP_GET_DISABLE, &dis,
	    "RAIDFRAME_PARITYMAP_GET_DISABLE");
	printf("raid%d: parity map will %s %sabled on next configure\n", 
	    raidID, dis == st.enabled ? "be" : "remain", dis ? "dis" : "en");
}

static void
rf_pm_configure(int fd, int raidID, char *parityconf, int parityparams[])
{
	int dis;
	struct rf_pmparams params;

	if (strcasecmp(parityconf, "yes") == 0)
		dis = 0;
	else if (strcasecmp(parityconf, "no") == 0)
		dis = 1;
	else if (strcasecmp(parityconf, "set") == 0) {
		params.cooldown = parityparams[0];
		params.tickms = parityparams[1];
		params.regions = parityparams[2];
		
		do_ioctl(fd, RAIDFRAME_PARITYMAP_SET_PARAMS, &params,
		    "RAIDFRAME_PARITYMAP_SET_PARAMS");

		if (params.cooldown != 0 || params.tickms != 0) {
			printf("raid%d: parity cleaned after", raidID);
			if (params.cooldown != 0)
				printf(" %d", params.cooldown);
			printf(" intervals");
			if (params.tickms != 0) {
				printf(" of %d.%03ds", params.tickms / 1000,
				    params.tickms % 1000);
			}
			printf("\n");
		}
		if (params.regions != 0)
			printf("raid%d: will use %d regions on next"
			    " configuration\n", raidID, params.regions);

		return;
		/* XXX the control flow here could be prettier. */
	} else
		err(1, "`%s' is not a valid parity map command", parityconf);

	do_ioctl(fd, RAIDFRAME_PARITYMAP_SET_DISABLE, &dis,
	    "RAIDFRAME_PARITYMAP_SET_DISABLE");
	printf("raid%d: parity map will be %sabled on next configure\n", 
	    raidID, dis ? "dis" : "en");
}


static void
rf_output_configuration(int fd, const char *name)
{
	RF_DeviceConfig_t device_config;
	void *cfg_ptr;
	int i;
	RF_ComponentLabel_t component_label;
	void *label_ptr;
	int component_num;
	int num_cols;

	cfg_ptr = &device_config;

	printf("# raidctl config file for %s\n", name);
	printf("\n");
	do_ioctl(fd, RAIDFRAME_GET_INFO, &cfg_ptr, "RAIDFRAME_GET_INFO");

	printf("START array\n");
	printf("# numRow numCol numSpare\n");
	printf("%d %d %d\n", device_config.rows, device_config.cols,
	    device_config.nspares);
	printf("\n");

	printf("START disks\n");
	for(i=0; i < device_config.ndevs; i++)
		printf("%s\n", device_config.devs[i].devname);
	printf("\n");

	if (device_config.nspares > 0) {
		printf("START spare\n");
		for(i=0; i < device_config.nspares; i++)
			printf("%s\n", device_config.spares[i].devname);
		printf("\n");
	}

	for(i=0; i < device_config.ndevs; i++) {
		if (device_config.devs[i].status == rf_ds_optimal)
			break;
	}
	if (i == device_config.ndevs) {
		printf("# WARNING: no optimal components; using %s\n",
		    device_config.devs[0].devname);
		i = 0;
	}
	get_component_number(fd, device_config.devs[i].devname,
	    &component_num, &num_cols);
	memset(&component_label, 0, sizeof(RF_ComponentLabel_t));
	component_label.row = component_num / num_cols;
	component_label.column = component_num % num_cols;
	label_ptr = &component_label;
	do_ioctl(fd, RAIDFRAME_GET_COMPONENT_LABEL, &label_ptr,
		  "RAIDFRAME_GET_COMPONENT_LABEL");

	printf("START layout\n");
	printf(
	    "# sectPerSU SUsPerParityUnit SUsPerReconUnit RAID_level_%c\n",
	    (char) component_label.parityConfig);
	printf("%d %d %d %c\n", 
	    component_label.sectPerSU, component_label.SUsPerPU, 
	    component_label.SUsPerRU, (char) component_label.parityConfig);
	printf("\n");

	printf("START queue\n");
	printf("fifo %d\n", device_config.maxqdepth);
}

static void
get_component_number(int fd, char *component_name, int *component_number,
		     int *num_columns)
{
	RF_DeviceConfig_t device_config;
	void *cfg_ptr;
	int i;
	int found;

	*component_number = -1;
		
	/* Assuming a full path spec... */
	cfg_ptr = &device_config;
	do_ioctl(fd, RAIDFRAME_GET_INFO, &cfg_ptr, 
		 "RAIDFRAME_GET_INFO");

	*num_columns = device_config.cols;
	
	found = 0;
	for(i=0; i < device_config.ndevs; i++) {
		if (strncmp(component_name, device_config.devs[i].devname,
			    PATH_MAX)==0) {
			found = 1;
			*component_number = i;
		}
	}
	if (!found) { /* maybe it's a spare? */
		for(i=0; i < device_config.nspares; i++) {
			if (strncmp(component_name, 
				    device_config.spares[i].devname,
				    PATH_MAX)==0) {
				found = 1;
				*component_number = i + device_config.ndevs;
				/* the way spares are done should
				   really change... */
				*num_columns = device_config.cols + 
					device_config.nspares;
			}
		}
	}

	if (!found)
		err(1,"%s is not a component of this device", component_name);
}

static void
rf_fail_disk(int fd, char *component_to_fail, int do_recon)
{
	struct rf_recon_req recon_request;
	int component_num;
	int num_cols;

	get_component_number(fd, component_to_fail, &component_num, &num_cols);

	recon_request.row = component_num / num_cols;
	recon_request.col = component_num % num_cols;
	if (do_recon) {
		recon_request.flags = RF_FDFLAGS_RECON;
	} else {
		recon_request.flags = RF_FDFLAGS_NONE;
	}
	do_ioctl(fd, RAIDFRAME_FAIL_DISK, &recon_request, 
		 "RAIDFRAME_FAIL_DISK");
	if (do_recon && verbose) {
		printf("Reconstruction status:\n");
		sleep(3); /* XXX give reconstruction a chance to start */
		do_meter(fd,RAIDFRAME_CHECK_RECON_STATUS_EXT);
	}
}

static void
get_component_label(int fd, char *component)
{
	RF_ComponentLabel_t component_label;
	void *label_ptr;
	int component_num;
	int num_cols;

	get_component_number(fd, component, &component_num, &num_cols);

	memset( &component_label, 0, sizeof(RF_ComponentLabel_t));
	component_label.row = component_num / num_cols;
	component_label.column = component_num % num_cols;

	label_ptr = &component_label;
	do_ioctl( fd, RAIDFRAME_GET_COMPONENT_LABEL, &label_ptr,
		  "RAIDFRAME_GET_COMPONENT_LABEL");

	printf("Component label for %s:\n",component);

	printf("   Row: %d, Column: %d, Num Rows: %d, Num Columns: %d\n",
	       component_label.row, component_label.column, 
	       component_label.num_rows, component_label.num_columns);
	printf("   Version: %d, Serial Number: %u, Mod Counter: %d\n",
	       component_label.version, component_label.serial_number,
	       component_label.mod_counter);
	printf("   Clean: %s, Status: %d\n",
	       component_label.clean ? "Yes" : "No", 
	       component_label.status );
	printf("   sectPerSU: %d, SUsPerPU: %d, SUsPerRU: %d\n",
	       component_label.sectPerSU, component_label.SUsPerPU, 
	       component_label.SUsPerRU);
	printf("   Queue size: %d, blocksize: %d, numBlocks: %"PRIu64"\n",
	       component_label.maxOutstanding, component_label.blockSize,
	       rf_component_label_numblocks(&component_label));
	printf("   RAID Level: %c\n", (char) component_label.parityConfig);
	printf("   Autoconfig: %s\n", 
	       component_label.autoconfigure ? "Yes" : "No" );
	printf("   Root partition: %s\n",
	       rootpart[component_label.root_partition & 3]);
	printf("   Last configured as: raid%d\n", component_label.last_unit );
}

static void
set_component_label(int fd, char *component)
{
	RF_ComponentLabel_t component_label;
	int component_num;
	int num_cols;

	get_component_number(fd, component, &component_num, &num_cols);

	/* XXX This is currently here for testing, and future expandability */

	component_label.version = 1;
	component_label.serial_number = 123456;
	component_label.mod_counter = 0;
	component_label.row = component_num / num_cols;
	component_label.column = component_num % num_cols;
	component_label.num_rows = 0;
	component_label.num_columns = 5;
	component_label.clean = 0;
	component_label.status = 1;
	
	do_ioctl( fd, RAIDFRAME_SET_COMPONENT_LABEL, &component_label,
		  "RAIDFRAME_SET_COMPONENT_LABEL");
}


static void
init_component_labels(int fd, int serial_number)
{
	RF_ComponentLabel_t component_label;

	component_label.version = 0;
	component_label.serial_number = serial_number;
	component_label.mod_counter = 0;
	component_label.row = 0;
	component_label.column = 0;
	component_label.num_rows = 0;
	component_label.num_columns = 0;
	component_label.clean = 0;
	component_label.status = 0;
	
	do_ioctl( fd, RAIDFRAME_INIT_LABELS, &component_label,
		  "RAIDFRAME_SET_COMPONENT_LABEL");
}

static void
set_autoconfig(int fd, int raidID, char *autoconf)
{
	int auto_config;
	int root_config;

	auto_config = 0;
	root_config = 0;

	if (strncasecmp(autoconf, "root", 4) == 0 ||
	    strncasecmp(autoconf, "hard", 4) == 0 ||
	    strncasecmp(autoconf, "force", 4) == 0) {
		root_config = 1;
	} else if (strncasecmp(autoconf, "soft", 4) == 0) {
		root_config = 2;
	}

	if ((strncasecmp(autoconf,"yes", 3) == 0) ||
	    root_config > 0) {
		auto_config = 1;
	}

	do_ioctl(fd, RAIDFRAME_SET_AUTOCONFIG, &auto_config,
		 "RAIDFRAME_SET_AUTOCONFIG");

	do_ioctl(fd, RAIDFRAME_SET_ROOT, &root_config,
		 "RAIDFRAME_SET_ROOT");

	printf("raid%d: Autoconfigure: %s\n", raidID,
	       auto_config ? "Yes" : "No");

	if (auto_config == 1) {
		printf("raid%d: Root: %s\n", raidID, rootpart[root_config]);
	}
}

static void
add_hot_spare(int fd, char *component)
{
	RF_SingleComponent_t hot_spare;

	hot_spare.row = 0;
	hot_spare.column = 0;
	strncpy(hot_spare.component_name, component, 
		sizeof(hot_spare.component_name));
	
	do_ioctl( fd, RAIDFRAME_ADD_HOT_SPARE, &hot_spare,
		  "RAIDFRAME_ADD_HOT_SPARE");
}

static void
remove_hot_spare(int fd, char *component)
{
	RF_SingleComponent_t hot_spare;
	int component_num;
	int num_cols;

	get_component_number(fd, component, &component_num, &num_cols);

	hot_spare.row = component_num / num_cols;
	hot_spare.column = component_num % num_cols;

	strncpy(hot_spare.component_name, component, 
		sizeof(hot_spare.component_name));
	
	do_ioctl( fd, RAIDFRAME_REMOVE_HOT_SPARE, &hot_spare,
		  "RAIDFRAME_REMOVE_HOT_SPARE");
}

static void
rebuild_in_place(int fd, char *component)
{
	RF_SingleComponent_t comp;
	int component_num;
	int num_cols;

	get_component_number(fd, component, &component_num, &num_cols);

	comp.row = 0;
	comp.column = component_num;
	strncpy(comp.component_name, component, sizeof(comp.component_name));
	
	do_ioctl( fd, RAIDFRAME_REBUILD_IN_PLACE, &comp,
		  "RAIDFRAME_REBUILD_IN_PLACE");

	if (verbose) {
		printf("Reconstruction status:\n");
		sleep(3); /* XXX give reconstruction a chance to start */
		do_meter(fd,RAIDFRAME_CHECK_RECON_STATUS_EXT);
	}

}

static void
check_parity(int fd, int do_rewrite, char *dev_name)
{
	int is_clean;
	int percent_done;

	is_clean = 0;
	percent_done = 0;
	do_ioctl(fd, RAIDFRAME_CHECK_PARITY, &is_clean,
		 "RAIDFRAME_CHECK_PARITY");
	if (is_clean) {
		printf("%s: Parity status: clean\n",dev_name);
	} else {
		printf("%s: Parity status: DIRTY\n",dev_name);
		if (do_rewrite) {
			printf("%s: Initiating re-write of parity\n",
			       dev_name);
			do_ioctl(fd, RAIDFRAME_REWRITEPARITY, NULL, 
				 "RAIDFRAME_REWRITEPARITY");
			sleep(3); /* XXX give it time to
				     get started. */
			if (verbose) {
				printf("Parity Re-write status:\n");
				do_meter(fd, RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT);
			} else {
				do_ioctl(fd, 
					 RAIDFRAME_CHECK_PARITYREWRITE_STATUS, 
					 &percent_done, 
					 "RAIDFRAME_CHECK_PARITYREWRITE_STATUS"
					 );
				while( percent_done < 100 ) {
					sleep(3); /* wait a bit... */
					do_ioctl(fd, RAIDFRAME_CHECK_PARITYREWRITE_STATUS, 
						 &percent_done, "RAIDFRAME_CHECK_PARITYREWRITE_STATUS");
				}

			}
			       printf("%s: Parity Re-write complete\n",
				      dev_name);
		} else {
			/* parity is wrong, and is not being fixed.
			   Exit w/ an error. */
			exit(1);
		}
	}
}


static void
check_status(int fd, int meter)
{
	int recon_percent_done = 0;
	int parity_percent_done = 0;
	int copyback_percent_done = 0;

	do_ioctl(fd, RAIDFRAME_CHECK_RECON_STATUS, &recon_percent_done, 
		 "RAIDFRAME_CHECK_RECON_STATUS");
	printf("Reconstruction is %d%% complete.\n", recon_percent_done);
	do_ioctl(fd, RAIDFRAME_CHECK_PARITYREWRITE_STATUS, 
		 &parity_percent_done, 
		 "RAIDFRAME_CHECK_PARITYREWRITE_STATUS");
	printf("Parity Re-write is %d%% complete.\n", parity_percent_done);
	do_ioctl(fd, RAIDFRAME_CHECK_COPYBACK_STATUS, &copyback_percent_done, 
		 "RAIDFRAME_CHECK_COPYBACK_STATUS");
	printf("Copyback is %d%% complete.\n", copyback_percent_done);

	if (meter) {
		/* These 3 should be mutually exclusive at this point */
		if (recon_percent_done < 100) {
			printf("Reconstruction status:\n");
			do_meter(fd,RAIDFRAME_CHECK_RECON_STATUS_EXT);
		} else if (parity_percent_done < 100) {
			printf("Parity Re-write status:\n");
			do_meter(fd,RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT);
		} else if (copyback_percent_done < 100) {
			printf("Copyback status:\n");
			do_meter(fd,RAIDFRAME_CHECK_COPYBACK_STATUS_EXT);
		}
	}
}

const char *tbits = "|/-\\";

static void
do_meter(int fd, u_long option)
{
	int percent_done;
	RF_uint64 start_value;
	RF_ProgressInfo_t progressInfo;
	void *pInfoPtr;
	struct timeval start_time;
	struct timeval current_time;
	double elapsed;
	int elapsed_sec;
	int elapsed_usec;
	int simple_eta,last_eta;
	double rate;
	RF_uint64 amount;
	int tbit_value;
	char bar_buffer[1024];
	char eta_buffer[1024];

	if (gettimeofday(&start_time,NULL) == -1)
		err(1, "gettimeofday failed!?!?");
	memset(&progressInfo, 0, sizeof(RF_ProgressInfo_t));
	pInfoPtr=&progressInfo;

	percent_done = 0;
	do_ioctl(fd, option, &pInfoPtr, "");
	start_value = progressInfo.completed;
	current_time = start_time;
	simple_eta = 0;
	last_eta = 0;
	
	tbit_value = 0;
	while(progressInfo.completed < progressInfo.total) {

		percent_done = (progressInfo.completed * 100) / 
			progressInfo.total;

		get_bar(bar_buffer, percent_done, 40);
		
		elapsed_sec = current_time.tv_sec - start_time.tv_sec;
		elapsed_usec = current_time.tv_usec - start_time.tv_usec;
		if (elapsed_usec < 0) {
			elapsed_usec-=1000000;
			elapsed_sec++;
		}

		elapsed = (double) elapsed_sec + 
			(double) elapsed_usec / 1000000.0;

		amount = progressInfo.completed - start_value;

		if (amount <= 0) { /* we don't do negatives (yet?) */
			amount = 0;
		}

		if (elapsed == 0)
			rate = 0.0;
		else
			rate = amount / elapsed;

		if (rate > 0.0) {
			simple_eta = (int) (((double)progressInfo.total - 
					     (double) progressInfo.completed) 
					    / rate);
		} else {
			simple_eta = -1;
		}

		if (simple_eta <=0) { 
			simple_eta = last_eta;
		} else {
			last_eta = simple_eta;
		}

		get_time_string(eta_buffer, simple_eta);

		fprintf(stdout,"\r%3d%% |%s| ETA: %s %c",
			percent_done,bar_buffer,eta_buffer,tbits[tbit_value]);
		fflush(stdout);

		if (++tbit_value>3) 
			tbit_value = 0;

		sleep(2);

		if (gettimeofday(&current_time,NULL) == -1)
			err(1, "gettimeofday failed!?!?");

		do_ioctl( fd, option, &pInfoPtr, "");
		

	}
	printf("\n");
}
/* 40 '*''s per line, then 40 ' ''s line. */
/* If you've got a screen wider than 160 characters, "tough" */

#define STAR_MIDPOINT 4*40
const char stars[] = "****************************************"
                     "****************************************"
                     "****************************************"
                     "****************************************"
                     "                                        "
                     "                                        "
                     "                                        "
                     "                                        "
                     "                                        ";

static void
get_bar(char *string, double percent, int max_strlen)
{
	int offset;

	if (max_strlen > STAR_MIDPOINT) {
		max_strlen = STAR_MIDPOINT;
	}
	offset = STAR_MIDPOINT - 
		(int)((percent * max_strlen)/ 100);
	if (offset < 0)
		offset = 0;
	snprintf(string,max_strlen,"%s",&stars[offset]);
}

static void
get_time_string(char *string, int simple_time)
{
	int minutes, seconds, hours;
	char hours_buffer[5];
	char minutes_buffer[5];
	char seconds_buffer[5];

	if (simple_time >= 0) {

		minutes = (int) simple_time / 60;
		seconds = ((int)simple_time - 60*minutes);
		hours = minutes / 60;
		minutes = minutes - 60*hours;
		
		if (hours > 0) {
			snprintf(hours_buffer,5,"%02d:",hours);
		} else {
			snprintf(hours_buffer,5,"   ");
		}
		
		snprintf(minutes_buffer,5,"%02d:",minutes);
		snprintf(seconds_buffer,5,"%02d",seconds);
		snprintf(string,1024,"%s%s%s",
			 hours_buffer, minutes_buffer, seconds_buffer);
	} else {
		snprintf(string,1024,"   --:--");
	}
	
}

static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "usage: %s [-v] -a component dev\n", progname);
	fprintf(stderr, "       %s [-v] -A [yes | no | softroot | hardroot] dev\n", progname);
	fprintf(stderr, "       %s [-v] -B dev\n", progname);
	fprintf(stderr, "       %s [-v] -c config_file dev\n", progname);
	fprintf(stderr, "       %s [-v] -C config_file dev\n", progname);
	fprintf(stderr, "       %s [-v] -f component dev\n", progname);
	fprintf(stderr, "       %s [-v] -F component dev\n", progname);
	fprintf(stderr, "       %s [-v] -g component dev\n", progname);
	fprintf(stderr, "       %s [-v] -G dev\n", progname);
	fprintf(stderr, "       %s [-v] -i dev\n", progname);
	fprintf(stderr, "       %s [-v] -I serial_number dev\n", progname);
	fprintf(stderr, "       %s [-v] -m dev\n", progname);
	fprintf(stderr, "       %s [-v] -M [yes | no | set params] dev\n",
	    progname);
	fprintf(stderr, "       %s [-v] -p dev\n", progname);
	fprintf(stderr, "       %s [-v] -P dev\n", progname);
	fprintf(stderr, "       %s [-v] -r component dev\n", progname); 
	fprintf(stderr, "       %s [-v] -R component dev\n", progname);
	fprintf(stderr, "       %s [-v] -s dev\n", progname);
	fprintf(stderr, "       %s [-v] -S dev\n", progname);
	fprintf(stderr, "       %s [-v] -u dev\n", progname);
	exit(1);
	/* NOTREACHED */
}
