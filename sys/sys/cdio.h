/*	$NetBSD: cdio.h,v 1.24 2006/02/02 14:48:02 reinoud Exp $	*/

#ifndef _SYS_CDIO_H_
#define _SYS_CDIO_H_

/* Shared between kernel & process */

union msf_lba {
	struct {
		u_char unused;
		u_char minute;
		u_char second;
		u_char frame;
	} msf;
	uint32_t lba;
	u_char	addr[4];
};

struct cd_toc_entry {
	u_char	nothing1;
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	control:4;
	u_char	addr_type:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_char	addr_type:4;
	u_char	control:4;
#endif
	u_char	track;
	u_char	nothing2;
	union msf_lba	addr;
};

struct cd_sub_channel_header {
	u_char	nothing1;
	u_char	audio_status;
#define CD_AS_AUDIO_INVALID	0x00
#define CD_AS_PLAY_IN_PROGRESS	0x11
#define CD_AS_PLAY_PAUSED	0x12
#define CD_AS_PLAY_COMPLETED	0x13
#define CD_AS_PLAY_ERROR	0x14
#define CD_AS_NO_STATUS		0x15
	u_char	data_len[2];
};

struct cd_sub_channel_q_data {
	u_char	data_format;
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	control:4;
	u_char	addr_type:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_char	addr_type:4;
	u_char	control:4;
#endif
	u_char	track_number;
	u_char	index_number;
	u_char	absaddr[4];
	u_char	reladdr[4];
#if BYTE_ORDER == LITTLE_ENDIAN
        u_char  :7;
        u_char  mc_valid:1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
        u_char  mc_valid:1;
        u_char  :7;
#endif
        u_char  mc_number[15];
#if BYTE_ORDER == LITTLE_ENDIAN
        u_char  :7;
        u_char  ti_valid:1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
        u_char  ti_valid:1;
        u_char  :7;
#endif
        u_char  ti_number[15];
};

struct cd_sub_channel_position_data {
	u_char	data_format;
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	control:4;
	u_char	addr_type:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_char	addr_type:4;
	u_char	control:4;
#endif
	u_char	track_number;
	u_char	index_number;
	union msf_lba	absaddr;
	union msf_lba	reladdr;
};

struct cd_sub_channel_media_catalog {
	u_char	data_format;
	u_char	nothing1;
	u_char	nothing2;
	u_char	nothing3;
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	:7;
	u_char	mc_valid:1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_char	mc_valid:1;
	u_char	:7;
#endif
	u_char	mc_number[15];
};

struct cd_sub_channel_track_info {
	u_char	data_format;
	u_char	nothing1;
	u_char	track_number;
	u_char	nothing2;
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	:7;
	u_char	ti_valid:1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_char	ti_valid:1;
	u_char	:7;
#endif
	u_char	ti_number[15];
};

struct cd_sub_channel_info {
	struct cd_sub_channel_header header;
	union {
		struct cd_sub_channel_q_data q_data;
		struct cd_sub_channel_position_data position;
		struct cd_sub_channel_media_catalog media_catalog;
		struct cd_sub_channel_track_info track_info;
	} what;
};

/*
 * Ioctls for the CD drive
 */
struct ioc_play_track {
	u_char	start_track;
	u_char	start_index;
	u_char	end_track;
	u_char	end_index;
};

#define	CDIOCPLAYTRACKS	_IOW('c', 1, struct ioc_play_track)
struct ioc_play_blocks {
	int	blk;
	int	len;
};
#define	CDIOCPLAYBLOCKS	_IOW('c', 2, struct ioc_play_blocks)

struct ioc_read_subchannel {
	u_char	address_format;
#define CD_LBA_FORMAT		1
#define CD_MSF_FORMAT		2
	u_char	data_format;
#define CD_SUBQ_DATA		0
#define CD_CURRENT_POSITION	1
#define CD_MEDIA_CATALOG	2
#define CD_TRACK_INFO		3
	u_char	track;
	int	data_len;
	struct	cd_sub_channel_info *data;
};
#define CDIOCREADSUBCHANNEL _IOWR('c', 3, struct ioc_read_subchannel )

struct ioc_toc_header {
	u_short	len;
	u_char	starting_track;
	u_char	ending_track;
};

#define CDIOREADTOCHEADER _IOR('c', 4, struct ioc_toc_header)

struct ioc_read_toc_entry {
	u_char	address_format;
	u_char	starting_track;
	u_short	data_len;
	struct	cd_toc_entry *data;
};
#define CDIOREADTOCENTRIES _IOWR('c', 5, struct ioc_read_toc_entry)
#define CDIOREADTOCENTRYS CDIOREADTOCENTRIES

/* read LBA start of a given session; 0=last, others not yet supported */
#define CDIOREADMSADDR _IOWR('c', 6, int)

struct	ioc_patch {
	u_char	patch[4];	/* one for each channel */
};
#define	CDIOCSETPATCH	_IOW('c', 9, struct ioc_patch)

struct	ioc_vol {
	u_char	vol[4];	/* one for each channel */
};
#define	CDIOCGETVOL	_IOR('c', 10, struct ioc_vol)
#define	CDIOCSETVOL	_IOW('c', 11, struct ioc_vol)
#define	CDIOCSETMONO	_IO('c', 12)
#define	CDIOCSETSTEREO	_IO('c', 13)
#define	CDIOCSETMUTE	_IO('c', 14)
#define	CDIOCSETLEFT	_IO('c', 15)
#define	CDIOCSETRIGHT	_IO('c', 16)
#define	CDIOCSETDEBUG	_IO('c', 17)
#define	CDIOCCLRDEBUG	_IO('c', 18)
#define	CDIOCPAUSE	_IO('c', 19)
#define	CDIOCRESUME	_IO('c', 20)
#define	CDIOCRESET	_IO('c', 21)
#define	CDIOCSTART	_IO('c', 22)
#define	CDIOCSTOP	_IO('c', 23)
#define	CDIOCEJECT	_IO('c', 24)
#define	CDIOCALLOW	_IO('c', 25)
#define	CDIOCPREVENT	_IO('c', 26)
#define	CDIOCCLOSE	_IO('c', 27)

struct ioc_play_msf {
	u_char	start_m;
	u_char	start_s;
	u_char	start_f;
	u_char	end_m;
	u_char	end_s;
	u_char	end_f;
};
#define	CDIOCPLAYMSF	_IOW('c', 25, struct ioc_play_msf)

struct ioc_load_unload {
	u_char options;
#define	CD_LU_ABORT	0x1	/* NOTE: These are the same as the ATAPI */
#define	CD_LU_UNLOAD	0x2	/* op values for the LOAD_UNLOAD command */
#define	CD_LU_LOAD	0x3
	u_char slot;
};
#define		CDIOCLOADUNLOAD	_IOW('c', 26, struct ioc_load_unload)


#if defined(_KERNEL) || defined(_EXPOSE_MMC)
/* not exposed to userland yet until its completely mature */
/*
 * MMC device abstraction interface.
 *
 * It gathers information from GET_CONFIGURATION, READ_DISCINFO,
 * READ_TRACKINFO, READ_TOC2, READ_CD_CAPACITY and GET_CONFIGURATION
 * SCSI/ATAPI calls regardless if its a legacy CD-ROM/DVD-ROM device or a MMC
 * standard recordable device.
 */
struct mmc_discinfo {
	uint16_t	mmc_profile;
	uint16_t	mmc_class;

	uint8_t		disc_state;
	uint8_t		last_session_state;
	uint8_t		bg_format_state;
	uint8_t		link_block_penalty;	/* in sectors		   */

	uint64_t	mmc_cur;		/* current MMC_CAPs        */
	uint64_t	mmc_cap;		/* possible MMC_CAPs       */

	uint32_t	disc_flags;		/* misc flags              */

	uint32_t	disc_id;
	uint64_t	disc_barcode;
	uint8_t		application_code;	/* 8 bit really            */

	uint8_t		unused1[3];		/* padding                 */

	uint32_t	last_possible_lba;	/* last leadout start adr. */
	uint32_t	sector_size;
	uint32_t	blockingnr;		/* ECC/write blocking size */

	uint16_t	num_sessions;
	uint16_t	num_tracks;		/* derived */

	uint16_t	first_track;
	uint16_t	first_track_last_session;
	uint16_t	last_track_last_session;

	uint16_t	unused2;		/* padding/misc info resv. */

	uint16_t	reserved1[4];		/* MMC-5 track resources   */
	uint32_t	reserved2[3];		/* MMC-5 POW resources     */

	uint32_t	reserved3[8];		/* MMC5+ */
};
#define MMCGETDISCINFO	_IOR('c', 28, struct mmc_discinfo)

#define MMC_CLASS_UNKN  0
#define MMC_CLASS_DISC	1
#define MMC_CLASS_CD	2
#define MMC_CLASS_DVD	3
#define MMC_CLASS_MO	4
#define MMC_CLASS_BD	5
#define MMC_CLASS_FILE	0xffff	/* emulation mode */

#define MMC_DFLAGS_BARCODEVALID	(1 <<  0)  /* barcode is present and valid   */
#define MMC_DFLAGS_DISCIDVALID  (1 <<  1)  /* discid is present and valid    */
#define MMC_DFLAGS_APPCODEVALID (1 <<  2)  /* application code valid         */
#define MMC_DFLAGS_UNRESTRICTED (1 <<  3)  /* restricted, then set app. code */

#define MMC_DFLAGS_FLAGBITS \
    "\10\1BARCODEVALID\2DISCIDVALID\3APPCODEVALID\4UNRESTRICTED"

#define MMC_CAP_SEQUENTIAL	(1 <<  0)  /* sequential writable only       */
#define MMC_CAP_RECORDABLE	(1 <<  1)  /* record-able; i.e. not static   */
#define MMC_CAP_ERASABLE	(1 <<  2)  /* drive can erase sectors        */
#define MMC_CAP_BLANKABLE	(1 <<  3)  /* media can be blanked           */
#define MMC_CAP_FORMATTABLE	(1 <<  4)  /* media can be formatted         */
#define MMC_CAP_REWRITABLE	(1 <<  5)  /* media can be rewritten         */
#define MMC_CAP_MRW		(1 <<  6)  /* Mount Rainier formatted        */
#define MMC_CAP_PACKET		(1 <<  7)  /* using packet recording         */
#define MMC_CAP_STRICTOVERWRITE	(1 <<  8)  /* only writes a packet at a time */
#define MMC_CAP_PSEUDOOVERWRITE (1 <<  9)  /* overwrite through replacement  */
#define MMC_CAP_ZEROLINKBLK	(1 << 10)  /* zero link block length capable */

#define MMC_CAP_FLAGBITS \
    "\10\1SEQUENTIAL\2RECORDABLE\3ERASABLE\4BLANKABLE\5FORMATTABLE" \
    "\6REWRITABLE\7MRW\10PACKET\11STRICTOVERWRITE\12PSEUDOOVERWRITE" \
    "\13ZEROLINKBLK"

#define MMC_STATE_EMPTY		0
#define MMC_STATE_INCOMPLETE	1
#define MMC_STATE_FULL		2
#define MMC_STATE_CLOSED	3

#define MMC_BGFSTATE_UNFORM	0
#define MMC_BGFSTATE_STOPPED	1
#define MMC_BGFSTATE_RUNNING	2
#define	MMC_BGFSTATE_COMPLETED	3


struct mmc_trackinfo {
	uint16_t	tracknr;	/* IN/OUT */
	uint16_t	sessionnr;

	uint8_t		track_mode;
	uint8_t		data_mode;

	uint16_t	flags;

	uint32_t	track_start;
	uint32_t	next_writable;
	uint32_t	free_blocks;
	uint32_t	packet_size;
	uint32_t	track_size;
	uint32_t	last_recorded;
};
#define MMCGETTRACKINFO	_IOWR('c', 29, struct mmc_trackinfo)

#define MMC_TRACKINFO_COPY		(1 << 0)
#define MMC_TRACKINFO_DAMAGED		(1 << 1)
#define MMC_TRACKINFO_FIXED_PACKET	(1 << 2)
#define MMC_TRACKINFO_INCREMENTAL	(1 << 3)
#define MMC_TRACKINFO_BLANK		(1 << 4)
#define MMC_TRACKINFO_RESERVED		(1 << 5)
#define MMC_TRACKINFO_NWA_VALID		(1 << 6)
#define MMC_TRACKINFO_LRA_VALID		(1 << 7)

#define MMC_TRACKINFO_FLAGBITS \
    "\8\1COPY\2DAMAGED\3FIXEDPACKET\4INCREMENTAL\5BLANK" \
    "\6RESERVED\7NWA_VALID\10LRA_VALID"

#endif /* _KERNEL || _EXPOSE_MMC */

#endif /* !_SYS_CDIO_H_ */
