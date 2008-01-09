/*	$NetBSD: io.c,v 1.1.6.2 2008/01/09 01:48:30 matt Exp $	*/


#include <lib/libsa/stand.h>
#include <sys/bswap.h>
#include "boot.h"

#define POW_IOCC_SEG 0x820C00E0
#define IOCC_SEG 0x82000080
#define PSL_DR  (1<<4)

volatile u_char *MCA_io  = (u_char *)0xe0000000;

/* hardcode for now */
int
setup_iocc(void)
{
	register_t savemsr, msr;

	__asm volatile ("mfmsr %0" : "=r"(savemsr));
	msr = savemsr & ~PSL_DR;
	__asm volatile ("mtmsr %0" : : "r"(msr));

	__asm volatile ("mtsr 14,%0" : : "r"(IOCC_SEG));
	__asm volatile ("mtmsr %0" : : "r"(msr|PSL_DR));
	__asm volatile ("isync");
	__asm volatile ("mtmsr %0;isync" : : "r"(savemsr));
	return 1;
}

void
outb(int port, char val)
{

	MCA_io[port] = val;
}

inline void
outw(int port, u_int16_t val)
{
        outb(port, val>>8);
        outb(port+1, val);
}

u_char
inb(int port)
{

	return (MCA_io[port]);
}
