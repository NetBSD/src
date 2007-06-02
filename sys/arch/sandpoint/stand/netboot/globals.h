/* $NetBSD: globals.h,v 1.1.2.1 2007/06/02 08:44:37 nisimura Exp $ */

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
unsigned pcicfgread(unsigned, unsigned);
void  pcicfgwrite(unsigned, unsigned, unsigned);

/* cache ops */
void _wb(uint32_t, uint32_t);
void _wbinv(uint32_t, uint32_t);
void _inv(uint32_t, uint32_t);

/* NIF */
void netif_init(void);
