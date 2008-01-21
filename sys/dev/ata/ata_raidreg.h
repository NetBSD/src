/*	$NetBSD: ata_raidreg.h,v 1.2.2.2 2008/01/21 09:42:37 yamt Exp $	*/

/*-
 * Copyright (c) 2000,2001,2002 Søren Schmidt <sos@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _DEV_PCI_PCIIDE_PROMISE_RAID_H_
#define	_DEV_PCI_PCIIDE_PROMISE_RAID_H_

/*
 * Macro to compute the LBA of the Promise RAID configuration structure,
 * using the disk's softc structure.
 */
#define	PR_LBA(wd)							\
	((((wd)->sc_capacity /						\
	   ((wd)->sc_params.atap_heads * (wd)->sc_params.atap_sectors)) * \
	  (wd)->sc_params.atap_heads * (wd)->sc_params.atap_sectors) -	\
	 (wd)->sc_params.atap_sectors)

struct promise_raid_conf {
	char		promise_id[24];
#define	PR_MAGIC	"Promise Technology, Inc."

	uint32_t	dummy_0;

	uint64_t	magic_0;
	uint16_t	magic_1;
	uint32_t	magic_2;
	uint8_t		filler1[470];
	struct {				/* 0x200 */
		uint32_t	integrity;
#define	PR_I_VALID	0x00000080

		uint8_t		flags;
#define	PR_F_VALID	0x01
#define	PR_F_ONLINE	0x02
#define	PR_F_ASSIGNED	0x04
#define	PR_F_SPARE	0x08
#define	PR_F_DUPLICATE	0x10
#define	PR_F_REDIR	0x20
#define	PR_F_DOWN	0x40
#define	PR_F_READY	0x80

		uint8_t		disk_number;
		uint8_t		channel;
		uint8_t		device;
		uint64_t	magic_0 __packed;
		uint32_t	disk_offset;	/* 0x210 */
		uint32_t	disk_sectors;
		uint32_t	rebuild_lba;
		uint16_t	generation;
		uint8_t		status;
#define	PR_S_VALID	0x01
#define	PR_S_ONLINE	0x02
#define	PR_S_INITED	0x04
#define	PR_S_READY	0x08
#define	PR_S_DEGRADED	0x10
#define	PR_S_MARKED	0x20
#define	PR_S_FUNCTIONAL	0x80

		uint8_t		type;
#define	PR_T_RAID0	0x00
#define	PR_T_RAID1	0x01
#define	PR_T_RAID3	0x02
#define	PR_T_RAID5	0x04
#define	PR_T_SPAN	0x08

		uint8_t		total_disks;	/* 0x220 */
		uint8_t		stripe_shift;
		uint8_t		array_width;
		uint8_t		array_number;
		uint32_t	total_sectors;
		uint16_t	cylinders;
		uint8_t		heads;
		uint8_t		sectors;
		uint64_t	magic_1 __packed;
		struct {
			uint8_t		flags;
			uint8_t		dummy_0;
			uint8_t		channel;
			uint8_t		device;
			uint64_t	magic_0 __packed;
		} disk[8];
	} raid;
	uint32_t	filler2[346];
	uint32_t	checksum;
} __packed;

/*
 * Macro to compute the LBA of the Adaptec HostRAID configuration structure,
 * using the disk's softc structure.
 */
#define	ADP_LBA(wd)							\
	((wd)->sc_capacity - 17)

struct adaptec_raid_conf {
	uint32_t	magic_0;
#define	ADP_MAGIC_0	0x900765c4

	uint32_t	generation;
	uint16_t	dummy_0;
	uint16_t	total_configs;
	uint16_t	dummy_1;
	uint16_t	checksum;
	uint32_t	dummy_2;
	uint32_t	dummy_3;
	uint32_t	flags;
	uint32_t	timestamp;
	uint32_t	dummy_4[4];
	uint32_t	dummy_5[4];
	struct {
		uint16_t	total_disks;
		uint16_t	generation;
		uint32_t	magic_0;
		uint8_t		dummy_0;
		uint8_t		type;
#define ADP_T_RAID0			0x00
#define ADP_T_RAID1			0x01
		uint8_t		dummy_1;
		uint8_t		flags;

		uint8_t		dummy_2;
		uint8_t		dummy_3;
		uint8_t		dummy_4;
		uint8_t		dummy_5;

		uint32_t	disk_number;
		uint32_t	dummy_6;
		uint32_t	sectors;
		uint16_t	stripe_sectors;
		uint16_t	dummy_7;

		uint32_t	dummy_8[4];
		uint8_t		name[16];
	} configs[127];
	uint32_t	dummy_6[13];
	uint32_t	magic_1;
#define ADP_MAGIC_1		0x0950f89f
	uint32_t	dummy_7[3];
	uint32_t	magic_2;
	uint32_t	dummy_8[46];
	uint32_t	magic_3;
#define ADP_MAGIC_3		0x4450544d
	uint32_t	magic_4;
#define ADP_MAGIC_4		0x0950f89f
	uint32_t	dummy_9[62];
} __packed;

/* VIA Tech V-RAID Metadata */
/* Derrived from FreeBSD ata-raid.h 1.46 */
#define VIA_LBA(wd) ((wd)->sc_capacity - 1)

struct via_raid_conf {
	uint16_t	magic;
#define VIA_MAGIC		0xaa55
	uint8_t		dummy_0;
	uint8_t		type;
#define VIA_T_MASK		0x7e
#define VIA_T_BOOTABLE		0x01
#define VIA_T_RAID0		0x04
#define VIA_T_RAID1		0x0c
#define VIA_T_RAID01		0x4c
#define VIA_T_RAID5		0x2c
#define VIA_T_SPAN		0x44
#define VIA_T_UNKNOWN		0x80
	uint8_t		disk_index;
#define VIA_D_MASK		0x0f
#define VIA_D_DEGRADED		0x10
#define VIA_D_HIGH_IDX		0x20
	uint8_t		stripe_layout;
#define VIA_L_DISKS		0x07
#define VIA_L_MASK		0xf0
#define VIA_L_SHIFT		4
	uint64_t		disk_sectors;
	uint32_t		disk_id;
	uint32_t		disks[8];
	uint8_t			checksum;
	uint8_t			pad_0[461];
} __packed;

#endif /* _DEV_PCI_PCIIDE_PROMISE_RAID_H_ */
