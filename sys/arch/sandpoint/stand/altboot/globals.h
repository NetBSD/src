/* $NetBSD: globals.h,v 1.20 2015/09/29 15:12:52 phx Exp $ */

#ifdef DEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

/* clock feed */
#ifndef EXT_CLK_FREQ
#define EXT_CLK_FREQ	33333333	/* external clock (PCI clock) */
#endif

/* brd type */
extern int brdtype;
#define BRD_SANDPOINTX2		2
#define BRD_SANDPOINTX3		3
#define BRD_ENCOREPP1		10
#define BRD_KUROBOX		100
#define BRD_QNAPTS		101
#define BRD_SYNOLOGY		102
#define BRD_STORCENTER		103
#define BRD_DLINKDSM		104
#define BRD_NH230NAS		105
#define BRD_KUROBOXT4		106
#define BRD_UNKNOWN		-1

struct brdprop {
	const char *family;
	const char *verbose;
	int brdtype;
	uint32_t extclk;
	char *consname;
	int consport;
	int consspeed;
	void (*setup)(struct brdprop *);
	void (*brdfix)(struct brdprop *);
	void (*pcifix)(struct brdprop *);
	void (*launch)(struct brdprop *);
	void (*reset)(void);
};

extern uint32_t cpuclock, busclock;

/* board specific support code */
struct brdprop *brd_lookup(int);
int get_drive_config(int);
int tstchar(void);
#ifdef DEBUG
void sat_write(char *, int);
int sat_getch(void);
int sat_tstch(void);
#endif
void read_mac_from_flash(uint8_t *);

/* PPC processor ctl */
void __syncicache(void *, size_t);

/* i/o access */
void out8(unsigned, unsigned);
unsigned in8(unsigned);
void out16rb(unsigned, unsigned);
void out32rb(unsigned, unsigned);
unsigned in16rb(unsigned);
unsigned in32rb(unsigned);
void iohtole16(unsigned, unsigned);
void iohtole32(unsigned, unsigned);
unsigned iole32toh(unsigned);
unsigned iole16toh(unsigned);

/* far call would never return */
void run(void *, void *, void *, void *, void *);

/* micro second precision delay */
void delay(unsigned);

/* PCI stuff */
struct pcidev {
	unsigned bdf;	/* bus.dev.func */
	unsigned pvd;	/* device ID */
	void *drv;	/* driver */
};
extern struct pcidev lata[2];
extern struct pcidev lnif[2];
extern struct pcidev lusb[3];
extern int nata, nnif, nusb;

void  pcisetup(void);
void  pcifixup(void);
void  launchfixup(void);
unsigned pcimaketag(int, int, int);
void  pcidecomposetag(unsigned, int *, int *, int *);
int   pcifinddev(unsigned, unsigned, unsigned *);
int   pcilookup(unsigned, struct pcidev *, int);
unsigned pcicfgread(unsigned, int);
void  pcicfgwrite(unsigned, int, unsigned);

#define PCI_ID_REG			0x00
#define  PCI_VENDOR(id)			((id) & 0xffff)
#define  PCI_PRODUCT(id)		(((id) >> 16) & 0xffff)
#define  PCI_VENDOR_INVALID		0xffff
#define  PCI_DEVICE(v,p)		((v) | ((p) << 16))
#define PCI_COMMAND_STATUS_REG		0x04
#define PCI_CLASS_REG			0x08
#define  PCI_CLASS(v)			(((v) >> 16) & 0xffff)
#define  PCI_SUBCLASS(v)		(((v) >> 16) & 0xff)
#define  PCI_INTERFACE(v)		(((v) & 0xff00) >> 8)
#define  PCI_REVISION(v)		((v) & 0xff)
#define  PCI_CLASS_PPB			0x0604
#define  PCI_CLASS_ETH			0x0200
#define  PCI_CLASS_SCSI			0x0100
#define  PCI_CLASS_IDE			0x0101
#define  PCI_CLASS_RAID			0x0104
#define  PCI_CLASS_SATA			0x0106
#define  PCI_CLASS_MISCSTORAGE		0x0180
#define  PCI_CLASS_USB			0x0c03
#define PCI_BHLC_REG			0x0c
#define  PCI_HDRTYPE_TYPE(r)		(((r) >> 16) & 0x7f)
#define  PCI_HDRTYPE_MULTIFN(r)		((r) & (0x80 << 16))

/*
 * "Map B" layout
 *
 * practice direct mode configuration scheme with CONFIG_ADDR
 * (0xfec0'0000) and CONFIG_DATA (0xfee0'0000).
 */
#define PCI_MEMBASE	0x80000000	/* PCI memory space */
#define PCI_MEMLIMIT	0xfbffffff	/* EUMB is next to this */
#define PCI_IOBASE	0x00001000	/* reserves room for southbridge */
#define PCI_IOLIMIT	0x000fffff
#define PCI_XIOBASE	0xfe000000	/* ISA/PCI io space */
#define CONFIG_ADDR	0xfec00000
#define CONFIG_DATA	0xfee00000

/* cache ops */
void _wb(uint32_t, uint32_t);
void _wbinv(uint32_t, uint32_t);
void _inv(uint32_t, uint32_t);

/* parsing */
uint32_t read_hex(const char *);

/* heap */
void *allocaligned(size_t, size_t);

/* NIF support */
int net_open(struct open_file *, ...);
int net_close(struct open_file *);
int net_strategy(void *, int, daddr_t, size_t, void *, size_t *);

int netif_init(void *);
void netif_shutdown_all(void);
int netif_open(void *); 
int netif_close(int); 

#define NIF_DECL(xxx) \
    int xxx ## _match(unsigned, void *); \
    void * xxx ## _init(unsigned, void *); \
    int xxx ## _send(void *, char *, unsigned); \
    int xxx ## _recv(void *, char *, unsigned, unsigned); \
    void xxx ## _shutdown(void *)

NIF_DECL(fxp);
NIF_DECL(tlp);
NIF_DECL(rge);
NIF_DECL(skg);
NIF_DECL(stg);

/* DSK support */
#define MAX_UNITS 4

struct disk {
	char xname[8];
	void *dvops;
	unsigned unitchan;
	unsigned unittag;
	uint16_t ident[128];
	uint64_t nsect;
	uint64_t first;
	void *dlabel;
	int part;
	void *fsops;
	int (*lba_read)(struct disk *, int64_t, int, void *);
};

int dskdv_init(void *);
int dlabel_valid(int);
int dsk_open(struct open_file *, ...);
int dsk_close(struct open_file *);
int dsk_strategy(void *, int, daddr_t, size_t, void *, size_t *);
struct fs_ops *dsk_fsops(struct open_file *);

#define DSK_DECL(xxx) \
    int xxx ## _match(unsigned, void *); \
    void * xxx ## _init(unsigned, void *)

DSK_DECL(pciide);
DSK_DECL(siisata);

/* status */
#define ATA_STS_BUSY		0x80
#define ATA_STS_DRDY		0x40
#define ATA_STS_ERR 		0x01
/* command */
#define ATA_CMD_CHKPWR		0xe5
#define ATA_CMD_IDENT		0xec
#define ATA_CMD_IDLE		0xe3
#define ATA_CMD_READ		0x20
#define ATA_CMD_READ_EXT	0x24
#define ATA_CMD_SETF		0xef
#define ATA_CMD_STANDBY		0xe2
/* device */
#define ATA_DEV_LBA		0xe0
#define ATA_DEV_OBS		0x90
/* control */
#define ATA_DREQ		0x08
#define ATA_SRST		0x04
/* power state */
#define ATA_PWR_ACTIVE		0xff
#define ATA_PWR_IDLE		0x80
#define ATA_PWR_STANDBY		0x00

#define ATA_XFER		0x03
#define XFER_PIO4		0x0c
#define XFER_PIO0		0x08

struct dvata_chan {
	uint32_t cmd, ctl, alt, dma;
};
#define _DAT	0	/* RW */
#define _ERR	1	/* R */
#define _FEA	1	/* W */
#define _NSECT	2	/* RW */
#define _LBAL	3	/* RW */
#define _LBAM	4	/* RW */
#define _LBAH	5	/* RW */
#define _DEV	6	/* W */
#define _STS	7	/* R */
#define _CMD	7	/* W */

struct dkdev_ata {
	unsigned tag;
	uint32_t bar[6];
	struct dvata_chan chan[4];
	int presense[4];
	char *iobuf;
};

int spinwait_unbusy(struct dkdev_ata *, int, int, const char **);
int perform_atareset(struct dkdev_ata *, int);
void wakeup_drive(struct dkdev_ata *, int);
int atachkpwr(struct dkdev_ata *, int);
