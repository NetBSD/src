/*	$NetBSD: atavar.h,v 1.27 2003/07/08 10:06:28 itojun Exp $	*/

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.
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
 *    must display the following acknowledgement:
 *	This product includes software developed by Manuel Bouyer.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 */

/* Hight-level functions and structures used by both ATA and ATAPI devices */

/* Datas common to drives and controller drivers */
struct ata_drive_datas {
    u_int8_t drive; /* drive number */
    int8_t ata_vers; /* ATA version supported */
    u_int16_t drive_flags; /* bitmask for drives present/absent and cap */
#define DRIVE_ATA	0x0001
#define DRIVE_ATAPI	0x0002
#define DRIVE_OLD	0x0004 
#define DRIVE (DRIVE_ATA|DRIVE_ATAPI|DRIVE_OLD)
#define DRIVE_CAP32	0x0008
#define DRIVE_DMA	0x0010 
#define DRIVE_UDMA	0x0020
#define DRIVE_MODE	0x0040 /* the drive reported its mode */
#define DRIVE_RESET	0x0080 /* reset the drive state at next xfer */
#define DRIVE_DMAERR	0x0100 /* Udma transfer had crc error, don't try DMA */
#define DRIVE_ATAPIST	0x0100 /* device is an ATAPI tape drive */
    /*
     * Current setting of drive's PIO, DMA and UDMA modes.
     * Is initialised by the disks drivers at attach time, and may be
     * changed later by the controller's code if needed
     */
    u_int8_t PIO_mode; /* Current setting of drive's PIO mode */
    u_int8_t DMA_mode; /* Current setting of drive's DMA mode */
    u_int8_t UDMA_mode; /* Current setting of drive's UDMA mode */
    /* Supported modes for this drive */
    u_int8_t PIO_cap; /* supported drive's PIO mode */
    u_int8_t DMA_cap; /* supported drive's DMA mode */
    u_int8_t UDMA_cap; /* supported drive's UDMA mode */
    /*
     * Drive state.
     * This is reset to 0 after a channel reset.
     */
    u_int8_t state;
#define RESET          0
#define RECAL          1
#define RECAL_WAIT     2
#define PIOMODE        3
#define PIOMODE_WAIT   4
#define DMAMODE        5
#define DMAMODE_WAIT   6
#define GEOMETRY       7
#define GEOMETRY_WAIT  8
#define MULTIMODE      9
#define MULTIMODE_WAIT 10
#define READY          11

    /* numbers of xfers and DMA errs. Used by ata_dmaerr() */
    u_int8_t n_dmaerrs;
    u_int32_t n_xfers;
    /* Downgrade after NERRS_MAX errors in at most NXFER xfers */
#define NERRS_MAX 4
#define NXFER 4000

    struct device *drv_softc; /* ATA drives softc, if any */
    void *chnl_softc; /* channel softc */
};

/* User config flags that force (or disable) the use of a mode */
#define ATA_CONFIG_PIO_MODES	0x0007
#define ATA_CONFIG_PIO_SET	0x0008
#define ATA_CONFIG_PIO_OFF	0
#define ATA_CONFIG_DMA_MODES	0x0070
#define ATA_CONFIG_DMA_SET	0x0080
#define ATA_CONFIG_DMA_DISABLE	0x0070
#define ATA_CONFIG_DMA_OFF	4
#define ATA_CONFIG_UDMA_MODES	0x0700
#define ATA_CONFIG_UDMA_SET	0x0800
#define ATA_CONFIG_UDMA_DISABLE	0x0700
#define ATA_CONFIG_UDMA_OFF	8

/*
 * ATA/ATAPI commands description 
 *
 * This structure defines the interface between the ATA/ATAPI device driver
 * and the controller for short commands. It contains the command's parameter,
 * the len of data's to read/write (if any), and a function to call upon
 * completion.
 * If no sleep is allowed, the driver can poll for command completion.
 * Once the command completed, if the error registed is valid, the flag
 * AT_ERROR is set and the error register value is copied to r_error .
 * A separate interface is needed for read/write or ATAPI packet commands
 * (which need multiple interrupts per commands).
 */
struct wdc_command {
    u_int8_t r_command;  /* Parameters to upload to registers */
    u_int8_t r_head;
    u_int16_t r_cyl;
    u_int8_t r_sector;
    u_int8_t r_count;
    u_int8_t r_precomp;
    u_int8_t r_st_bmask; /* status register mask to wait for before command */
    u_int8_t r_st_pmask; /* status register mask to wait for after command */
    u_int8_t r_error;    /* error register after command done */
    volatile u_int16_t flags;
#define AT_READ     0x0001 /* There is data to read */
#define AT_WRITE    0x0002 /* There is data to write (excl. with AT_READ) */
#define AT_WAIT     0x0008 /* wait in controller code for command completion */
#define AT_POLL     0x0010 /* poll for command completion (no interrupts) */
#define AT_DONE     0x0020 /* command is done */
#define AT_XFDONE   0x0040 /* data xfer is done */
#define AT_ERROR    0x0080 /* command is done with error */
#define AT_TIMEOU   0x0100 /* command timed out */
#define AT_DF       0x0200 /* Drive fault */
#define AT_READREG  0x0400 /* Read registers on completion */
    int timeout;	 /* timeout (in ms) */
    void *data;          /* Data buffer address */
    int bcount;           /* number of bytes to transfer */
    void (*callback) __P((void *)); /* command to call once command completed */
    void *callback_arg;  /* argument passed to *callback() */
};

/*
 * If WDSM_ATTR_ADVISORY, device exceeded intended design life period.
 * If not WDSM_ATTR_ADVISORY, imminent data loss predicted.
 */
#define WDSM_ATTR_ADVISORY	1
/*
 * If WDSM_ATTR_COLLECTIVE, attribute only updated in off-line testing.
 * If not WDSM_ATTR_COLLECTIVE, attribute updated also in on-line testing.
 */
#define WDSM_ATTR_COLLECTIVE	2

struct ata_smart_attr {
	u_int8_t		id;		/* attribute id number */
	u_int16_t		flags;
	u_int8_t		value;		/* attribute value */
	u_int8_t		vendor_specific[8];
} __attribute__((packed));

struct ata_smart_attributes {
	u_int16_t		data_structure_revision;
	struct ata_smart_attr	attributes[30];
	u_int8_t		offline_data_collection_status;
	u_int8_t		self_test_exec_status;
	u_int16_t		total_time_to_complete_off_line;
	u_int8_t		vendor_specific_366;
	u_int8_t		offline_data_collection_capability;
	u_int16_t		smart_capability;
	u_int8_t		errorlog_capability;
	u_int8_t		vendor_specific_371;
	u_int8_t		short_test_completion_time;
	u_int8_t		extend_test_completion_time;
	u_int8_t		reserved_374_385[12];
	u_int8_t		vendor_specific_386_509[125];
	int8_t			checksum;
} __attribute__((packed));

struct ata_smart_thresh {
	u_int8_t		id;
	u_int8_t		value;
	u_int8_t		reserved[10];
} __attribute__((packed));

struct ata_smart_thresholds {
	u_int16_t		data_structure_revision;
	struct ata_smart_thresh	thresholds[30];
	u_int8_t		reserved[18];
	u_int8_t		vendor_specific[131];
	int8_t			checksum;
} __attribute__((packed));

int  wdc_downgrade_mode __P((struct ata_drive_datas *));

struct ataparams;
int ata_get_params __P((struct ata_drive_datas *, u_int8_t,
	 struct ataparams *));
int ata_set_mode __P((struct ata_drive_datas *, u_int8_t, u_int8_t));
/* return code for these cmds */
#define CMD_OK    0
#define CMD_ERR   1
#define CMD_AGAIN 2

void ata_dmaerr __P((struct ata_drive_datas *));
