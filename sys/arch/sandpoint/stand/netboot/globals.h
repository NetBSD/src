/* $NetBSD: globals.h,v 1.6.4.4 2009/07/18 14:52:55 yamt Exp $ */

/* clock feed */
#ifndef TICKS_PER_SEC
#define TICKS_PER_SEC   (100000000 / 4)          /* 100MHz front bus */
#endif
#define NS_PER_TICK     (1000000000 / TICKS_PER_SEC)

/* brd type */
extern int brdtype;
#define BRD_SANDPOINTX2		2
#define BRD_SANDPOINTX3		3
#define BRD_ENCOREPP1		10
#define BRD_KUROBOX		100
#define BRD_UNKNOWN		-1

extern char *consname;
extern int consport;
extern int consspeed;
extern int ticks_per_sec;

unsigned mpc107memsize(void);

/* PPC processor ctl */
void __syncicache(void *, size_t);

/* byte swap access */
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
void  pcisetup(void);
void  pcifixup(void);
unsigned pcimaketag(int, int, int);
void  pcidecomposetag(unsigned, int *, int *, int *);
int   pcifinddev(unsigned, unsigned, unsigned *);
int   pcilookup(unsigned, unsigned [][2], int);
unsigned pcicfgread(unsigned, int);
void  pcicfgwrite(unsigned, int, unsigned);

#define PCI_ID_REG			0x00
#define PCI_COMMAND_STATUS_REG		0x04
#define  PCI_VENDOR(id)			((id) & 0xffff)
#define  PCI_PRODUCT(id)		(((id) >> 16) & 0xffff)
#define  PCI_VENDOR_INVALID		0xffff
#define  PCI_DEVICE(v,p)		((v) | ((p) << 16))
#define PCI_CLASS_REG			0x08
#define  PCI_CLASS_PPB			0x0604
#define  PCI_CLASS_ETH			0x0200
#define  PCI_CLASS_IDE			0x0101
#define  PCI_CLASS_RAID			0x0104
#define  PCI_CLASS_SATA			0x0106
#define PCI_BHLC_REG			0x0c
#define  PCI_HDRTYPE_TYPE(r)		(((r) >> 16) & 0x7f)
#define  PCI_HDRTYPE_MULTIFN(r)		((r) & (0x80 << 16))

/* cache ops */
void _wb(uint32_t, uint32_t);
void _wbinv(uint32_t, uint32_t);
void _inv(uint32_t, uint32_t);

/* NIF */
int net_open(struct open_file *, ...);
int net_close(struct open_file *);
int net_strategy(void *, int, daddr_t, size_t, void *, size_t *);

int netif_init(unsigned);
int netif_open(void *); 
int netif_close(int); 

#define NIF_DECL(xxx) \
    int xxx ## _match(unsigned, void *); \
    void * xxx ## _init(unsigned, void *); \
    int xxx ## _send(void *, char *, unsigned); \
    int xxx ## _recv(void *, char *, unsigned, unsigned)

NIF_DECL(fxp);
NIF_DECL(tlp);
NIF_DECL(nvt);
NIF_DECL(sip);
NIF_DECL(pcn);
NIF_DECL(kse);
NIF_DECL(sme);
NIF_DECL(vge);
NIF_DECL(rge);
NIF_DECL(wm);

#ifdef LABELSECTOR
/* IDE/SATA and disk */
int wdopen(struct open_file *, ...);
int wdclose(struct open_file *);
int wdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int parsefstype(void *);

struct atac_channel {
	volatile uint8_t *c_cmdbase;
	volatile uint8_t *c_ctlbase;
	volatile uint8_t *c_cmdreg[8 + 2];
	volatile uint16_t *c_data;
	int compatchan;
#define WDC_READ_CMD(chp, reg)		*(chp)->c_cmdreg[(reg)]
#define WDC_WRITE_CMD(chp, reg, val)	*(chp)->c_cmdreg[(reg)] = (val)
#define WDC_READ_CTL(chp, reg)		(chp)->c_ctlbase[(reg)]
#define WDC_WRITE_CTL(chp, reg, val)	(chp)->c_ctlbase[(reg)] = (val)
#define WDC_READ_DATA(chp)		*(chp)->c_data
};

struct atac_command {
	uint8_t drive;		/* drive id */
	uint8_t r_command;	/* Parameters to upload to registers */
	uint8_t r_head;
	uint16_t r_cyl;
	uint8_t r_sector;
	uint8_t r_count;
	uint8_t r_precomp;
	uint16_t bcount;
	void *data;
	uint64_t r_blkno;
};

struct atac_softc {
	unsigned tag;
	unsigned chvalid;
	struct atac_channel channel[4];
};

struct wd_softc {
#define WDF_LBA		0x0001
#define WDF_LBA48	0x0002
	int sc_flags;
	int sc_unit, sc_part;
	uint64_t sc_capacity;
	struct atac_channel *sc_channel;
	struct atac_command sc_command;
	struct ataparams sc_params;
	struct disklabel sc_label;
	uint8_t sc_buf[DEV_BSIZE];
};
#endif
