/*	$NetBSD: nvme.h,v 1.3 2018/04/18 10:11:44 nonaka Exp $	*/

/*-
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
 *
 * $FreeBSD: head/sys/dev/nvme/nvme.h 314888 2017-03-07 23:02:59Z imp $
 */

#ifndef __NVME_H__
#define __NVME_H__

/* Cap nvme to 1MB transfers driver explodes with larger sizes */
#define NVME_MAX_XFER_SIZE		(MAXPHYS < (1<<20) ? MAXPHYS : (1<<20))

/* Get Log Page */
#define	NVME_LOG_ERROR					0x01
#define	NVME_LOG_HEALTH_INFORMATION			0x02
#define	NVME_LOG_FIRMWARE_SLOT				0x03
#define	NVME_LOG_CHANGED_NAMESPACE_LIST			0x04
#define	NVME_LOG_COMMAND_EFFECTS_LOG			0x05
#define	NVME_LOG_RESERVATION_NOTIFICATION		0x80
/*
 * The following are Intel Specific log pages, but they seem
 * to be widely implemented.
 */
#define	INTEL_LOG_READ_LAT_LOG				0xc1
#define	INTEL_LOG_WRITE_LAT_LOG				0xc2
#define	INTEL_LOG_TEMP_STATS				0xc5
#define	INTEL_LOG_ADD_SMART				0xca
#define	INTEL_LOG_DRIVE_MKT_NAME			0xdd
/*
 * HGST log page, with lots ofs sub pages.
 */
#define	HGST_INFO_LOG					0xc1

/* Error Information Log (Log Identifier 01h) */
struct nvme_error_information_entry {
	uint64_t		error_count;
	uint16_t		sqid;
	uint16_t		cid;
	uint16_t		status;
	uint16_t		error_location;
	uint64_t		lba;
	uint32_t		nsid;
	uint8_t			vendor_specific;
	uint8_t			_reserved1[3];
	uint64_t		command_specific;
	uint8_t			reserved[24];
} __packed __aligned(4);

/* SMART / Health Information Log (Log Identifier 02h) */
struct nvme_health_information_page {
	uint8_t			critical_warning;
#define	NVME_HEALTH_PAGE_CW_VOLATILE_MEMORY_BACKUP	__BIT(4)
#define	NVME_HEALTH_PAGE_CW_READ_ONLY			__BIT(3)
#define	NVME_HEALTH_PAGE_CW_DEVICE_RELIABLITY		__BIT(2)
#define	NVME_HEALTH_PAGE_CW_TEMPERTURE			__BIT(1)
#define	NVME_HEALTH_PAGE_CW_AVAIL_SPARE			__BIT(0)
	uint16_t		composite_temperature;
	uint8_t			available_spare;
	uint8_t			available_spare_threshold;
	uint8_t			percentage_used;

	uint8_t			_reserved1[26];

	uint64_t		data_units_read[2];
	uint64_t		data_units_written[2];
	uint64_t		host_read_commands[2];
	uint64_t		host_write_commands[2];
	uint64_t		controller_busy_time[2];
	uint64_t		power_cycles[2];
	uint64_t		power_on_hours[2];
	uint64_t		unsafe_shutdowns[2];
	uint64_t		media_errors[2];
	uint64_t		num_error_info_log_entries[2];
	uint32_t		warning_temp_time;
	uint32_t		error_temp_time;
	uint16_t		temp_sensor[8];

	uint8_t			reserved[296];
} __packed __aligned(4);

/* Firmware Commit */
#define	NVME_COMMIT_ACTION_REPLACE_NO_ACTIVATE	0
#define	NVME_COMMIT_ACTION_REPLACE_ACTIVATE	1
#define	NVME_COMMIT_ACTION_ACTIVATE_RESET	2
#define	NVME_COMMIT_ACTION_ACTIVATE_NO_RESET	3

/* Firmware Slot Information (Log Identifier 03h) */
struct nvme_firmware_page {
	uint8_t			afi;
#define	NVME_FW_PAGE_AFI_SLOT_RST	__BITS(4, 6)
#define	NVME_FW_PAGE_AFI_SLOT		__BITS(0, 2)
	uint8_t			_reserved1[7];

	uint64_t		revision[7];	/* revisions for 7 slots */

	uint8_t			reserved[448];
} __packed __aligned(4);

struct intel_log_temp_stats {
	uint64_t	current;
	uint64_t	overtemp_flag_last;
	uint64_t	overtemp_flag_life;
	uint64_t	max_temp;
	uint64_t	min_temp;
	uint64_t	_rsvd[5];
	uint64_t	max_oper_temp;
	uint64_t	min_oper_temp;
	uint64_t	est_offset;
} __packed __aligned(4);

/* Commands Supported and Effects (Log Identifier 05h) */
struct nvme_command_effeects_page {
	uint32_t		acs[256];
#define	NVME_CE_PAGE_CS_CSE		__BITS(16, 18)
#define	NVME_CE_PAGE_CS_CCC		__BIT(4)
#define	NVME_CE_PAGE_CS_NIC		__BIT(3)
#define	NVME_CE_PAGE_CS_NCC		__BIT(2)
#define	NVME_CE_PAGE_CS_LBCC		__BIT(1)
#define	NVME_CE_PAGE_CS_CSUPP		__BIT(0)
	uint32_t		iocs[256];

	uint8_t			reserved[2048];
} __packed __aligned(4);

#endif	/* __NVME_H__ */
