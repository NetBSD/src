/*	$NetBSD: logpage.c,v 1.2.4.1 2017/04/21 16:53:14 bouyer Exp $	*/

/*-
 * Copyright (c) 2013 EMC Corp.
 * All rights reserved.
 *
 * Copyright (C) 2012-2013 Intel Corporation
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: logpage.c,v 1.2.4.1 2017/04/21 16:53:14 bouyer Exp $");
#if 0
__FBSDID("$FreeBSD: head/sbin/nvmecontrol/logpage.c 285796 2015-07-22 16:10:29Z jimharris $");
#endif
#endif

#include <sys/param.h>
#include <sys/ioccom.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nvmectl.h"
#include "bn.h"

#define DEFAULT_SIZE	(4096)
#define MAX_FW_SLOTS	(7)

typedef void (*print_fn_t)(void *buf, uint32_t size);

static void *
get_log_buffer(uint32_t size)
{
	void	*buf;

	if ((buf = malloc(size)) == NULL)
		errx(1, "unable to malloc %u bytes", size);

	memset(buf, 0, size);
	return (buf);
}

void
read_logpage(int fd, uint8_t log_page, int nsid, void *payload,
    uint32_t payload_size)
{
	struct nvme_pt_command	pt;

	memset(&pt, 0, sizeof(pt));
	pt.cmd.opcode = NVM_ADMIN_GET_LOG_PG;
	pt.cmd.nsid = nsid;
	pt.cmd.cdw10 = ((payload_size/sizeof(uint32_t)) - 1) << 16;
	pt.cmd.cdw10 |= log_page;
	pt.buf = payload;
	pt.len = payload_size;
	pt.is_read = 1;

	if (ioctl(fd, NVME_PASSTHROUGH_CMD, &pt) < 0)
		err(1, "get log page request failed");

	if (nvme_completion_is_error(&pt.cpl))
		errx(1, "get log page request returned error");
}

static void
print_log_error(void *buf, uint32_t size)
{
	int					i, nentries;
	struct nvme_error_information_entry	*entry = buf;

	printf("Error Information Log\n");
	printf("=====================\n");

	if (entry->error_count == 0) {
		printf("No error entries found\n");
		return;
	}

	nentries = size/sizeof(struct nvme_error_information_entry);
	for (i = 0; i < nentries; i++, entry++) {
		if (entry->error_count == 0)
			break;

		printf("Entry %02d\n", i + 1);
		printf("=========\n");
		printf(" Error count:           %ju\n", entry->error_count);
		printf(" Submission queue ID:   %u\n", entry->sqid);
		printf(" Command ID:            %u\n", entry->cid);
		/* TODO: Export nvme_status_string structures from kernel? */
		printf(" Status:\n");
		printf("  Phase tag:            %d\n",
		    (uint16_t)__SHIFTOUT(entry->status, NVME_CQE_PHASE));
		printf("  Status code:          %d\n",
		    (uint16_t)__SHIFTOUT(entry->status, NVME_CQE_SC_MASK));
		printf("  Status code type:     %d\n",
		    (uint16_t)__SHIFTOUT(entry->status, NVME_CQE_SCT_MASK));
		printf("  More:                 %d\n",
		    (uint16_t)__SHIFTOUT(entry->status, NVME_CQE_M));
		printf("  DNR:                  %d\n",
		    (uint16_t)__SHIFTOUT(entry->status, NVME_CQE_DNR));
		printf(" Error location:        %u\n", entry->error_location);
		printf(" LBA:                   %ju\n", entry->lba);
		printf(" Namespace ID:          %u\n", entry->nsid);
		printf(" Vendor specific info:  %u\n", entry->vendor_specific);
		printf(" Command specific info: %ju\n",
		    entry->command_specific);
	}
}

#define	METRIX_PREFIX_BUFSIZ	17
#define	NO_METRIX_PREFIX_BUFSIZ	42

static void
print_bignum(const char *title, uint64_t v[2], const char *suffix)
{
	char buf[64];
	uint8_t tmp[16];
	uint64_t l = le64toh(v[0]);
	uint64_t h = le64toh(v[1]);

	tmp[ 0] = (h >> 56) & 0xff;
	tmp[ 1] = (h >> 48) & 0xff;
	tmp[ 2] = (h >> 40) & 0xff;
	tmp[ 3] = (h >> 32) & 0xff;
	tmp[ 4] = (h >> 24) & 0xff;
	tmp[ 5] = (h >> 16) & 0xff;
	tmp[ 6] = (h >> 8) & 0xff;
	tmp[ 7] = h & 0xff;
	tmp[ 8] = (l >> 56) & 0xff;
	tmp[ 9] = (l >> 48) & 0xff;
	tmp[10] = (l >> 40) & 0xff;
	tmp[11] = (l >> 32) & 0xff;
	tmp[12] = (l >> 24) & 0xff;
	tmp[13] = (l >> 16) & 0xff;
	tmp[14] = (l >> 8) & 0xff;
	tmp[15] = l & 0xff;

	buf[0] = '\0';
	BIGNUM *bn = BN_bin2bn(tmp, __arraycount(tmp), NULL);
	if (bn != NULL) {
		humanize_bignum(buf, METRIX_PREFIX_BUFSIZ + strlen(suffix),
		    bn, suffix, HN_AUTOSCALE, HN_DECIMAL);
		BN_free(bn);
	}
	if (buf[0] == '\0')
		snprintf(buf, sizeof(buf), "0x%016jx%016jx", h, l);
	printf("%-31s %s\n", title, buf);
}

static void
print_log_health(void *buf, uint32_t size __unused)
{
	struct nvme_health_information_page *health = buf;
	float composite_temperature = health->composite_temperature;

	printf("SMART/Health Information Log\n");
	printf("============================\n");

	printf("Critical Warning State:         0x%02x\n",
	    health->critical_warning);
	printf(" Available spare:               %d\n",
	    (uint8_t)__SHIFTOUT(health->critical_warning,
	      NVME_HEALTH_PAGE_CW_AVAIL_SPARE));
	printf(" Temperature:                   %d\n",
	    (uint8_t)__SHIFTOUT(health->critical_warning,
	      NVME_HEALTH_PAGE_CW_TEMPERTURE));
	printf(" Device reliability:            %d\n",
	    (uint8_t)__SHIFTOUT(health->critical_warning,
	      NVME_HEALTH_PAGE_CW_DEVICE_RELIABLITY));
	printf(" Read only:                     %d\n",
	    (uint8_t)__SHIFTOUT(health->critical_warning,
	      NVME_HEALTH_PAGE_CW_READ_ONLY));
	printf(" Volatile memory backup:        %d\n",
	    (uint8_t)__SHIFTOUT(health->critical_warning,
	      NVME_HEALTH_PAGE_CW_VOLATILE_MEMORY_BACKUP));
	printf("Temperature:                    %u K, %2.2f C, %3.2f F\n",
	    health->composite_temperature,
	    composite_temperature - (float)273.15,
	    (composite_temperature * (float)9/5) - (float)459.67);
	printf("Available spare:                %u\n",
	    health->available_spare);
	printf("Available spare threshold:      %u\n",
	    health->available_spare_threshold);
	printf("Percentage used:                %u\n",
	    health->percentage_used);

	print_bignum("Data units (512 byte) read:",
	    health->data_units_read, "");
	print_bignum("Data units (512 byte) written:",
	    health->data_units_written, "");
	print_bignum("Host read commands:",
	    health->host_read_commands, "");
	print_bignum("Host write commands:",
	    health->host_write_commands, "");
	print_bignum("Controller busy time (minutes):",
	    health->controller_busy_time, "");
	print_bignum("Power cycles:",
	    health->power_cycles, "");
	print_bignum("Power on hours:",
	    health->power_on_hours, "");
	print_bignum("Unsafe shutdowns:",
	    health->unsafe_shutdowns, "");
	print_bignum("Media errors:",
	    health->media_errors, "");
	print_bignum("No. error info log entries:",
	    health->num_error_info_log_entries, "");
}

static void
print_log_firmware(void *buf, uint32_t size __unused)
{
	u_int				i;
	const char			*status;
	struct nvme_firmware_page	*fw = buf;

	printf("Firmware Slot Log\n");
	printf("=================\n");

	for (i = 0; i < MAX_FW_SLOTS; i++) {
		printf("Slot %d: ", i + 1);
		if (__SHIFTOUT(fw->afi, NVME_FW_PAGE_AFI_SLOT) == i + 1)
			status = "  Active";
		else
			status = "Inactive";

		if (fw->revision[i] == 0LLU)
			printf("Empty\n");
		else
			if (isprint(*(uint8_t *)&fw->revision[i]))
				printf("[%s] %.8s\n", status,
				    (char *)&fw->revision[i]);
			else
				printf("[%s] %016jx\n", status,
				    fw->revision[i]);
	}
}

static struct logpage_function {
	uint8_t		log_page;
	print_fn_t	fn;
} logfuncs[] = {
	{NVME_LOG_ERROR,		print_log_error		},
	{NVME_LOG_HEALTH_INFORMATION,	print_log_health	},
	{NVME_LOG_FIRMWARE_SLOT,	print_log_firmware	},
	{0,				NULL			},
};

__dead static void
logpage_usage(void)
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, LOGPAGE_USAGE);
	exit(1);
}

void
logpage(int argc, char *argv[])
{
	int				fd, nsid;
	int				log_page = 0, pageflag = false;
	int				hexflag = false, ns_specified;
	int				ch;
	char				*p;
	char				cname[64];
	uint32_t			size;
	void				*buf;
	struct logpage_function		*f;
	struct nvm_identify_controller	cdata;
	print_fn_t			print_fn;

	while ((ch = getopt(argc, argv, "p:x")) != -1) {
		switch (ch) {
		case 'p':
			/* TODO: Add human-readable ASCII page IDs */
			log_page = strtol(optarg, &p, 0);
			if (p != NULL && *p != '\0') {
				fprintf(stderr,
				    "\"%s\" not valid log page id.\n",
				    optarg);
				logpage_usage();
			/* TODO: Define valid log page id ranges in nvme.h? */
			} else if (log_page == 0 ||
				   (log_page >= 0x04 && log_page <= 0x7F) ||
				   (log_page >= 0x80 && log_page <= 0xBF)) {
				fprintf(stderr,
				    "\"%s\" not valid log page id.\n",
				    optarg);
				logpage_usage();
			}
			pageflag = true;
			break;
		case 'x':
			hexflag = true;
			break;
		}
	}

	if (!pageflag) {
		printf("Missing page_id (-p).\n");
		logpage_usage();
	}

	/* Check that a controller and/or namespace was specified. */
	if (optind >= argc)
		logpage_usage();

	if (strstr(argv[optind], NVME_NS_PREFIX) != NULL) {
		ns_specified = true;
		parse_ns_str(argv[optind], cname, &nsid);
		open_dev(cname, &fd, 1, 1);
	} else {
		ns_specified = false;
		nsid = 0xffffffff;
		open_dev(argv[optind], &fd, 1, 1);
	}

	read_controller_data(fd, &cdata);

	/*
	 * The log page attribtues indicate whether or not the controller
	 * supports the SMART/Health information log page on a per
	 * namespace basis.
	 */
	if (ns_specified) {
		if (log_page != NVME_LOG_HEALTH_INFORMATION)
			errx(1, "log page %d valid only at controller level",
			    log_page);
		if (!(cdata.lpa & NVME_ID_CTRLR_LPA_NS_SMART))
			errx(1,
			    "controller does not support per namespace "
			    "smart/health information");
	}

	print_fn = print_hex;
	if (!hexflag) {
		/*
		 * See if there is a pretty print function for the
		 *  specified log page.  If one isn't found, we
		 *  just revert to the default (print_hex).
		 */
		f = logfuncs;
		while (f->log_page > 0) {
			if (log_page == f->log_page) {
				print_fn = f->fn;
				break;
			}
			f++;
		}
	}

	/* Read the log page */
	switch (log_page) {
	case NVME_LOG_ERROR:
		size = sizeof(struct nvme_error_information_entry);
		size *= (cdata.elpe + 1);
		break;
	case NVME_LOG_HEALTH_INFORMATION:
		size = sizeof(struct nvme_health_information_page);
		break;
	case NVME_LOG_FIRMWARE_SLOT:
		size = sizeof(struct nvme_firmware_page);
		break;
	default:
		size = DEFAULT_SIZE;
		break;
	}

	buf = get_log_buffer(size);
	read_logpage(fd, log_page, nsid, buf, size);
	print_fn(buf, size);

	close(fd);
	exit(0);
}
