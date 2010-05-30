/* $NetBSD: globals.h,v 1.11.4.1 2010/05/30 05:17:05 rmind Exp $ */

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
#define BRD_QNAPTS101		101
#define BRD_SYNOLOGY		102
#define BRD_STORCENTER		103
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
	void (*reset)(void);
};

extern char *consname;
extern int consport;
extern int consspeed;
extern int ticks_per_sec;
extern uint32_t cpuclock, busclock;

/* board specific support code */
struct brdprop *brd_lookup(int);
unsigned mpc107memsize(void);
void read_mac_from_flash(uint8_t *);

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
#define  PCI_CLASS_MISCSTORAGE		0x0180
#define PCI_BHLC_REG			0x0c
#define  PCI_HDRTYPE_TYPE(r)		(((r) >> 16) & 0x7f)
#define  PCI_HDRTYPE_MULTIFN(r)		((r) & (0x80 << 16))

/* cache ops */
void _wb(uint32_t, uint32_t);
void _wbinv(uint32_t, uint32_t);
void _inv(uint32_t, uint32_t);

/* heap */
void *allocaligned(size_t, size_t);

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
NIF_DECL(rge);
NIF_DECL(skg);
