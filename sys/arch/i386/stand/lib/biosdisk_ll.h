/*	$NetBSD: biosdisk_ll.h,v 1.8 2003/04/16 12:43:45 dsl Exp $	 */

/*
 * Copyright (c) 1996
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
 * 	Perry E. Metzger.  All rights reserved.
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
 *    must display the following acknowledgements:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 *	This product includes software developed for the NetBSD Project
 *	by Perry E. Metzger.
 * 4. The names of the authors may not be used to endorse or promote products
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

/*
 * shared by bootsector startup (bootsectmain) and biosdisk.c needs lowlevel
 * parts from bios_disk.S
 */

/*
 * Beware that bios_disk.S relies on the offsets of the structure
 * members.
 */
struct biosdisk_ll {
	int             dev;		/* BIOS device number */
	int             sec, head, cyl;	/* geometry */
	int		flags;		/* see below */
	int		chs_sectors;	/* # of sectors addressable by CHS */
};
#define	BIOSDISK_EXT13	1		/* BIOS supports int13 extension */

#if __GNUC__ == 2 && __GNUC_MINOR__ < 7
#pragma pack(1)
#endif

/*
 * Version 1.x drive parameters from int13 extensions
 * - should be supported by every BIOS that supports the extensions.
 * Version 3.x parameters allow the drives to be matched properly
 * - but are much less likely to be supported.
 */

struct biosdisk_ext13info {
	u_int16_t	size;		/* size of buffer, set on call */
	u_int16_t	flags;		/* flags, see below */
	u_int32_t	cyl;		/* # of physical cylinders */
	u_int32_t	head;		/* # of physical heads */
	u_int32_t	sec;		/* # of physical sectors per track */
	u_int64_t	totsec;		/* total number of sectors */
	u_int16_t	sbytes;		/* # of bytes per sector */
#if defined(BIOSDISK_EXT13INFO_V2) || defined(BIOSDISK_EXT13INFO_V3)
	/* v2.0 extensions */
	u_int32_t	edd_cfg;	/* EDD configuration parameters */
#if defined(BIOSDISK_EXT13INFO_V3)
	/* v3.0 extensions */
	u_int16_t	devpath_sig;	/* 0xbedd if path info present */
#define EXT13_DEVPATH_SIGNATURE	0xbedd
	u_int8_t	devpath_len;	/* length from devpath_sig */
	u_int8_t	fill21[3];
	char		host_bus[4];	/* Probably "ISA" or "PCI" */
	char		iface_type[8];	/* "ATA", "ATAPI", "SCSI" etc */
	union {
		u_int8_t	ip_8[8];
		u_int16_t	ip_16[4];
		u_int32_t	ip_32[2];
		u_int64_t	ip_64[1];
	} interface_path;
#define	ip_isa_iobase	ip_16[0];	/* iobase for ISA bus */
#define	ip_pci_bus	ip_8[0];	/* PCI bus number */
#define	ip_pci_device	ip_8[1];	/* PCI device number */
#define	ip_pci_function	ip_8[2];	/* PCI function number */
	union {
		u_int8_t	dp_8[8];
		u_int16_t	dp_16[4];
		u_int32_t	dp_32[2];
		u_int64_t	dp_64[1];
	} device_path;
#define	dp_ata_slave	dp_8[0];
#define	dp_atapi_slave	dp_8[0];
#define	dp_atapi_lun	dp_8[1];
#define	dp_scsi_lun	dp_8[0];
#define	dp_firewire_guid dp_64[0];
#define	dp_fibrechnl_wwn dp_64[0];
	u_int8_t		fill40[1];
	u_int8_t		checksum;	/* byte sum from dev_path_sig is 0 */
#endif /* BIOSDISK_EXT13INFO_V3 */
#endif /* BIOSDISK_EXT13INFO_V2 */
} __attribute__((packed));

#define EXT13_DMA_TRANS		0x0001	/* transparent DMA boundary errors */
#define EXT13_GEOM_VALID	0x0002	/* geometry in c/h/s in struct valid */
#define EXT13_REMOVABLE		0x0004	/* removable device */
#define EXT13_WRITEVERF		0x0008	/* supports write with verify */
#define EXT13_CHANGELINE	0x0010	/* changeline support */
#define EXT13_LOCKABLE		0x0020	/* device is lockable */
#define EXT13_MAXGEOM		0x0040	/* geometry set to max; no media */

#if __GNUC__ == 2 && __GNUC_MINOR__ < 7
#pragma pack(4)
#endif

#define BIOSDISK_SECSIZE 512

int set_geometry __P((struct biosdisk_ll *, struct biosdisk_ext13info *));
int readsects   __P((struct biosdisk_ll *, daddr_t, int, char *, int));
