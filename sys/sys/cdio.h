/*	$NetBSD: cdio.h,v 1.19 2005/01/31 20:25:54 reinoud Exp $	*/

#ifndef _SYS_CDIO_H_
#define _SYS_CDIO_H_

/* Shared between kernel & process */

union msf_lba {
	struct {
		uint8_t unused;
		uint8_t minute;
		uint8_t second;
		uint8_t frame;
	} msf;
	u_int32_t lba;
	uint8_t	addr[4];
};

struct cd_toc_entry {
	uint8_t		nothing1;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t		control:4;
	uint8_t		addr_type:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t		addr_type:4;
	uint8_t		control:4;
#endif
	uint8_t		track;
	uint8_t		nothing2;
	union msf_lba	addr;
} __packed;

/* READSUBCHANNEL read definitions */
struct cd_sub_channel_header {
	uint8_t	nothing1;
	uint8_t	audio_status;
#define CD_AS_AUDIO_INVALID	0x00
#define CD_AS_PLAY_IN_PROGRESS	0x11
#define CD_AS_PLAY_PAUSED	0x12
#define CD_AS_PLAY_COMPLETED	0x13
#define CD_AS_PLAY_ERROR	0x14
#define CD_AS_NO_STATUS		0x15
	uint8_t	data_len[2];
};


struct cd_sub_channel_q_data {
	uint8_t	data_format;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t	control:4;
	uint8_t	addr_type:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t	addr_type:4;
	uint8_t	control:4;
#endif
	uint8_t	track_number;
	uint8_t	index_number;
	uint8_t	absaddr[4];
	uint8_t	reladdr[4];
#if BYTE_ORDER == LITTLE_ENDIAN
        uint8_t  :7;
        uint8_t  mc_valid:1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
        uint8_t  mc_valid:1;
        uint8_t  :7;
#endif
        uint8_t  mc_number[15]; 
#if BYTE_ORDER == LITTLE_ENDIAN
        uint8_t  :7;
        uint8_t  ti_valid:1;   
#endif
#if BYTE_ORDER == BIG_ENDIAN
        uint8_t  ti_valid:1;   
        uint8_t  :7;
#endif
        uint8_t  ti_number[15]; 
};


struct cd_sub_channel_position_data {
	uint8_t	data_format;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t	control:4;
	uint8_t	addr_type:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t	addr_type:4;
	uint8_t	control:4;
#endif
	uint8_t	track_number;
	uint8_t	index_number;
	union msf_lba	absaddr;
	union msf_lba	reladdr;
};


struct cd_sub_channel_media_catalog {
	uint8_t	data_format;
	uint8_t	nothing1;
	uint8_t	nothing2;
	uint8_t	nothing3;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t	:7;
	uint8_t	mc_valid:1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t	mc_valid:1;
	uint8_t	:7;
#endif
	uint8_t	mc_number[15];
};


struct cd_sub_channel_track_info {
	uint8_t	data_format;
	uint8_t	nothing1;
	uint8_t	track_number;
	uint8_t	nothing2;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t	:7;
	uint8_t	ti_valid:1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t	ti_valid:1;
	uint8_t	:7;
#endif
	uint8_t	ti_number[15];
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
	uint8_t	start_track;
	uint8_t	start_index;
	uint8_t	end_track;
	uint8_t	end_index;
};

#define	CDIOCPLAYTRACKS	_IOW('c', 1, struct ioc_play_track)
struct ioc_play_blocks {
	int	blk;
	int	len;
};
#define	CDIOCPLAYBLOCKS	_IOW('c', 2, struct ioc_play_blocks)

struct ioc_read_subchannel {
	uint8_t	address_format;
#define CD_LBA_FORMAT		1
#define CD_MSF_FORMAT		2
	uint8_t	data_format;
#define CD_SUBQ_DATA		0
#define CD_CURRENT_POSITION	1
#define CD_MEDIA_CATALOG	2
#define CD_TRACK_INFO		3
	uint8_t	 track;
	uint16_t data_len;
	struct cd_sub_channel_info *data;
};
#define CDIOCREADSUBCHANNEL _IOWR('c', 3, struct ioc_read_subchannel )

struct ioc_toc_header {
	uint16_t len;
	uint8_t	 starting_track;
	uint8_t	 ending_track;
};

#define CDIOREADTOCHEADER _IOR('c', 4, struct ioc_toc_header)

struct ioc_read_toc_entry {
	uint8_t	 address_format;
	uint8_t	 starting_track;
	uint16_t data_len;
	struct cd_toc_entry *data;
};
#define CDIOREADTOCENTRIES _IOWR('c', 5, struct ioc_read_toc_entry)
#define CDIOREADTOCENTRYS CDIOREADTOCENTRIES

/* read LBA start of a given session; 0=last, others not yet supported */
#define CDIOREADMSADDR _IOWR('c', 6, int)

struct	ioc_patch {
	uint8_t	patch[4];	/* one for each channel */
};
#define	CDIOCSETPATCH	_IOW('c', 9, struct ioc_patch)

struct	ioc_vol {
	uint8_t	vol[4];	/* one for each channel */
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
	uint8_t	start_m;
	uint8_t	start_s;
	uint8_t	start_f;
	uint8_t	end_m;
	uint8_t	end_s;
	uint8_t	end_f;
};
#define	CDIOCPLAYMSF	_IOW('c', 25, struct ioc_play_msf)

struct ioc_load_unload {
	uint8_t options;
#define	CD_LU_ABORT	0x1	/* NOTE: These are the same as the ATAPI */
#define	CD_LU_UNLOAD	0x2	/* op values for the LOAD_UNLOAD command */
#define	CD_LU_LOAD	0x3
	uint8_t slot;
};
#define		CDIOCLOADUNLOAD	_IOW('c', 26, struct ioc_load_unload)

#endif /* !_SYS_CDIO_H_ */
