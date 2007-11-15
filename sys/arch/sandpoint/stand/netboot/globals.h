/* $NetBSD: globals.h,v 1.2.4.3 2007/11/15 11:43:20 yamt Exp $ */

/* clock feed */
#define TICKS_PER_SEC   (100000000 / 4)          /* 100MHz front bus */
#define NS_PER_TICK     (1000000000 / TICKS_PER_SEC)

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

/* micro second precision delay */
void delay(unsigned);

/* PCI stuff */
void  pcisetup(void);
unsigned  pcimaketag(int, int, int);
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
#define PCI_CLASS_REG			0x08
#define  PCI_CLASS_PPB			0x0604
#define  PCI_CLASS_ETH			0x0200
#define  PCI_CLASS_IDE			0x0101
#define PCI_BHLC_REG			0x0c
#define  PCI_HDRTYPE_TYPE(r)		(((r) >> 16) & 0x7f)
#define  PCI_HDRTYPE_MULTIFN(r)		((r) & (0x80 << 16))

/* cache ops */
void _wb(uint32_t, uint32_t);
void _wbinv(uint32_t, uint32_t);
void _inv(uint32_t, uint32_t);

/* NIF */
int netif_init(unsigned);
