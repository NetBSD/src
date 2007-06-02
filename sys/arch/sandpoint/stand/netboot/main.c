/* $NetBSD: main.c,v 1.1.2.1 2007/06/02 08:44:37 nisimura Exp $ */

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <machine/bootinfo.h>

#include "globals.h"

uint8_t en[6];	/* NIC macaddr, fill by netif_init() */
void *bootinfo; /* low memory reserved to pass bootinfo structures */
int bi_size;	/* BOOTINFO_MAXSIZE */
char *bi_next;

extern char bootfile[];			/* filled by DHCP */
const char rootdev[] = "fxp";		/* Intel 82559 */

const unsigned dcache_line_size = 32;		/* 32B linesize */
const unsigned dcache_range_size = 4 * 1024;	/* 16KB / 4-way */

void main(void);
void bi_init(void *);
void bi_add(void *, int, int);
void _wb(unsigned, unsigned);
void _wbinv(unsigned, unsigned);
void _inv(unsigned, unsigned);
unsigned mpc107memsize(void);
void setup_82C686B(void);
void setup_83C533F(void);
void pcifixup(void);

void run(void *, void *, void *, void *, void *);

extern char bootprog_rev[], bootprog_maker[], bootprog_date[];

int brdtype;
#define BRD_SANDPOINTX2		2
#define BRD_SANDPOINTX3		3
#define BRD_ENCOREPP1		10
#define BRD_UNKNOWN		-1

void
main()
{
	int howto;
	unsigned memsize, tag;
	unsigned long marks[MARK_MAX];
	struct btinfo_memory bi_mem;
	struct btinfo_console bi_cons;
	struct btinfo_clock bi_clk;
	struct btinfo_bootpath bi_path;
	struct btinfo_rootdevice bi_rdev;

	/* determine SDRAM size */
	memsize = mpc107memsize();

	printf("\n");
	printf(">> NetBSD/sandpoint Boot, Revision %s\n", bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	switch (brdtype) {
	case BRD_SANDPOINTX3:
		printf("Sandpoint X3"); break;
	case BRD_ENCOREPP1:
		printf("Encore PP1"); break;
	}
	printf(", %dMB SDRAM", memsize >> 20);
	if (pcifinddev(0x8086, 0x1209, &tag) == 0
	    || pcifinddev(0x8086, 0x1229, &tag) == 0) {
		int b, d, f;
		pcidecomposetag(tag, &b, &d, &f);
		printf(", Intel i82559 NIC %02d:%02d:%02d", b, d, f);
	}
	printf("\n");

	pcisetup();
	pcifixup();

	netif_init();

	printf("Try NFS load /netbsd\n");
	marks[MARK_START] = 0;
	if (loadfile("net:", marks, LOAD_KERNEL) < 0) {
		printf("load failed. Restarting...\n");
		_rtt();
	}

	howto = RB_SINGLE | AB_VERBOSE | RB_KDB;

	bootinfo = (void *)0x4000;
	bi_init(bootinfo);

	bi_mem.memsize = memsize;
	snprintf(bi_cons.devname, sizeof(bi_cons.devname), CONSNAME);
	bi_cons.addr = CONSPORT;
	bi_cons.speed = CONSSPEED;
	bi_clk.ticks_per_sec = TICKS_PER_SEC;
	snprintf(bi_path.bootpath, sizeof(bi_path.bootpath), bootfile);
	snprintf(bi_rdev.devname, sizeof(bi_rdev.devname), rootdev);
	bi_rdev.cookie = tag; /* PCI tag for fxp netboot case */

	bi_add(&bi_cons, BTINFO_CONSOLE, sizeof(bi_cons));
	bi_add(&bi_mem, BTINFO_MEMORY, sizeof(bi_mem));
	bi_add(&bi_clk, BTINFO_CLOCK, sizeof(bi_clk));
	bi_add(&bi_path, BTINFO_BOOTPATH, sizeof(bi_path));
	bi_add(&bi_rdev, BTINFO_ROOTDEVICE, sizeof(bi_rdev));

	printf("entry=%p, ssym=%p, esym=%p\n",
	    (void *)marks[MARK_ENTRY],
	    (void *)marks[MARK_SYM],
	    (void *)marks[MARK_END]);

	__syncicache((void *)marks[MARK_ENTRY],
	    (u_int)marks[MARK_SYM] - (u_int)marks[MARK_ENTRY]);

	run((void *)marks[MARK_SYM], (void *)marks[MARK_END],
	    (void *)howto, bootinfo, (void *)marks[MARK_ENTRY]);

	/* should never come here */
	printf("exec returned. Restarting...\n");
	_rtt();
}

unsigned uartbase;
#define THR		0
#define DLB		0
#define DMB		1
#define IER		1
#define FCR		2
#define LCR		3
#define MCR		4
#define LSR		5
#define DCR		11
#define LSR_THRE	0x20
#define UART_READ(r)		*(volatile char *)(uartbase + (r))
#define UART_WRITE(r, v)	*(volatile char *)(uartbase + (r)) = (v)

void
brdsetup()
{
	unsigned pchb, pcib;

	/* BAT to arrange address space */

	/* EUMBBAR */
	pchb = pcimaketag(0, 0, 0);
	pcicfgwrite(pchb, 0x78, 0xfc000000);

	brdtype = BRD_UNKNOWN;
	if (pcifinddev(0x10ad, 0x0565, &pcib) == 0) {
		brdtype = BRD_SANDPOINTX3;
		setup_83C533F();
	}
	else if (pcifinddev(0x1106, 0x0686, &pcib) == 0) {
		brdtype = BRD_ENCOREPP1;
		setup_82C686B();
	}

	/* now prepare serial console */
	if (strcmp(CONSNAME, "eumb") != 0)
		uartbase = 0xfe000000 + CONSPORT; /* 0x3f8, 0x2f8 */
	else {
		uartbase = 0xfc000000 + CONSPORT; /* 0x4500, 0x4600 */
		UART_WRITE(DCR, 0x01);	/* 2 independent UART */
		UART_WRITE(LCR, 0x80);	/* turn on DLAB bit */
		UART_WRITE(FCR, 0x00);
		UART_WRITE(DMB, 0x00);
		UART_WRITE(DLB, 0x36);	/* 115200bps @ 100MHz, DINK32 sez */
		UART_WRITE(LCR, 0x03);	/* 8 N 1 */
		UART_WRITE(MCR, 0x03);	/* RTS DTR */
		UART_WRITE(FCR, 0x07);	/* FIFO_EN | RXSR | TXSR */
		UART_WRITE(IER, 0x00);	/* make sure INT disabled */
	}
}

void
putchar(c)
	int c;
{
	unsigned timo, lsr;

	if (c == '\n')
		putchar('\r');

	timo = 0x00100000;
	do {
		lsr = UART_READ(LSR);
	} while (timo-- > 0 && (lsr & LSR_THRE) == 0);
	if (timo > 0)
		UART_WRITE(THR, c);
}

void
_rtt()
{
	run(0, 0, 0, 0, (void *)0xFFF00100); /* reset entry */
	/* NOTREACHED */
}

static inline u_quad_t
mftb()
{
	u_long scratch;
	u_quad_t tb;

	asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw %0,%1; bne 1b"
	    : "=r"(tb), "=r"(scratch));
	return (tb);
}

time_t
getsecs()
{
	u_quad_t tb = mftb();

	return (tb / TICKS_PER_SEC);
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(n)
	u_int n;
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;

	tb = mftb();
	tb += (n * 1000 + NS_PER_TICK - 1) / NS_PER_TICK;
	tbh = tb >> 32;
	tbl = tb;
	asm volatile ("1: mftbu %0; cmpw %0,%1; blt 1b; bgt 2f; mftb %0; cmpw 0, %0,%2; blt 1b; 2:" : "=&r"(scratch) : "r"(tbh), "r"(tbl));
}

void
bi_init(addr)
	void *addr;
{
	struct btinfo_magic bi_magic;

	memset(addr, 0, BOOTINFO_MAXSIZE);
	bi_next = (char *)addr;
	bi_size = 0;

	bi_magic.magic = BOOTINFO_MAGIC;
	bi_add(&bi_magic, BTINFO_MAGIC, sizeof(bi_magic));
}

void
bi_add(new, type, size)
	void *new;
	int type, size;
{
	struct btinfo_common *bi;

	if (bi_size + size > BOOTINFO_MAXSIZE)
		return;				/* XXX error? */

	bi = new;
	bi->next = size;
	bi->type = type;
	memcpy(bi_next, new, size);
	bi_next += size;
}

void
_wb(adr, siz)
	uint32_t adr, siz;
{
	uint32_t off, bnd;

	asm volatile ("eieio");
	off = adr & (dcache_line_size - 1);
	adr -= off;
	siz += off;
	if (siz > dcache_range_size)
		siz = dcache_range_size;
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size)
		asm volatile ("dcbst 0,%0" :: "r"(adr));
	asm volatile ("sync");
}

void
_wbinv(adr, siz)
	uint32_t adr, siz;
{
	uint32_t off, bnd;

	asm volatile ("eieio");
	off = adr & (dcache_line_size - 1);
	adr -= off;
	siz += off;
	if (siz > dcache_range_size)
		siz = dcache_range_size;
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size)
		asm volatile ("dcbf 0,%0" :: "r"(adr));
	asm volatile ("sync");
}

void
_inv(adr, siz)
	uint32_t adr, siz;
{
	uint32_t off, bnd;

	/*
	 * NB - if adr and/or adr + siz are not cache line
	 * aligned, cache contents of the 1st and last cache line
	 * which do not belong to the invalidating range will be
	 * lost siliently. It's caller's responsibility to wb()
	 * them in the case.
	 */
	asm volatile ("eieio");
	off = adr & (dcache_line_size - 1);
	adr -= off;
	siz += off;
	if (siz > dcache_range_size)
		siz = dcache_range_size;
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size)
		asm volatile ("dcbi 0,%0" :: "r"(adr));
	asm volatile ("sync");
}

#include <dev/ic/mpc106reg.h>

unsigned
mpc107memsize()
{
	unsigned tag, val, n, bankn, end;

	tag = pcimaketag(0, 0, 0);

	if (brdtype == BRD_ENCOREPP1) {
		/* the brd's PPCBOOT looks to have erroneous values */
		unsigned tbl[] = {
#define	BK0_S	0x00000000
#define	BK0_E	(128 << 20) - 1
#define BK1_S	0x3ff00000
#define BK1_E	0x3fffffff
#define BK2_S	0x3ff00000
#define BK2_E	0x3fffffff
#define BK3_S	0x3ff00000
#define BK3_E	0x3fffffff
#define AR(v, s) ((((v) & SAR_MASK) >> SAR_SHIFT) << (s))
#define XR(v, s) ((((v) & EAR_MASK) >> EAR_SHIFT) << (s))
#define SAR_MASK 0x0ff00000
#define SAR_SHIFT    20
#define EAR_MASK 0x30000000
#define EAR_SHIFT    28
		AR(BK0_S, 0) | AR(BK1_S, 8) | AR(BK2_S, 16) | AR(BK3_S, 24),
		XR(BK0_S, 0) | XR(BK1_S, 8) | XR(BK2_S, 16) | XR(BK3_S, 24),
		AR(BK0_E, 0) | AR(BK1_E, 8) | AR(BK2_E, 16) | AR(BK3_E, 24),
		XR(BK0_E, 0) | XR(BK1_E, 8) | XR(BK2_E, 16) | XR(BK3_E, 24),
		};
		tag = pcimaketag(0, 0, 0);
		pcicfgwrite(tag, MPC106_MEMSTARTADDR1, tbl[0]);
		pcicfgwrite(tag, MPC106_EXTMEMSTARTADDR1, tbl[1]);
		pcicfgwrite(tag, MPC106_MEMENDADDR1, tbl[2]);
		pcicfgwrite(tag, MPC106_EXTMEMENDADDR1,	tbl[3]);
		pcicfgwrite(tag, MPC106_MEMEN, 1);
	}

	bankn = 0;
	val = pcicfgread(tag, MPC106_MEMEN);
	for (n = 0; n < 4; n++) {
		if ((val & (1U << n)) == 0)
			break;
		bankn = n;
	}
	bankn = bankn * 8;

	val = pcicfgread(tag, MPC106_EXTMEMENDADDR1);
	end =  ((val >> bankn) & 0x03) << 28;
	val = pcicfgread(tag, MPC106_MEMENDADDR1);
	end |= ((val >> bankn) & 0xff) << 20;
	end |= 0xfffff;

	return (end + 1); /* size of bankN SDRAM */
}

/*
 * VIA82C686B Southbridge
 *	0.22.0	1106.0686	PCI-ISA bridge
 *	0.22.1	1106.0571	IDE (viaide)
 *	0.22.2	1106.3038	USB 0/1 (uhci)
 *	0.22.3	1106.3038	USB 2/3 (uhci)
 *	0.22.4	1106.3057	power management
 *	0.22.5	1106.3058	AC97 (auvia)
 */
void
setup_82C686B()
{
	unsigned pcib, ide, usb12, usb34, ac97, pmgt, val;

	pcib  = pcimaketag(0, 22, 0);
	ide   = pcimaketag(0, 22, 1);
	usb12 = pcimaketag(0, 22, 2);
	usb34 = pcimaketag(0, 22, 3);
	pmgt  = pcimaketag(0, 22, 4);
	ac97  = pcimaketag(0, 22, 5);

	/* route USB (pin C) to i8259 IRQ 5, AC97 (pin D) to 11 */
	/* from kernel/arch/ppc/kernel/encpp1/encpp1_pci.c */

	val = pcicfgread(pcib, 0x54);
	val = (val & 0xff) | 0xb0500000; /* Dx CB Ax xS */
	pcicfgwrite(pcib, 0x54, val);

	/* enable EISA ELCR1 (0x4d0) and ELCR2 (0x4d1) */
	val = pcicfgread(pcib, 0x44);
	val = val | 0x20000000;
	pcicfgwrite(pcib, 0x44, val);

	/* select level trigger for IRQ 5/11 with ELCR1/ELCR2 */
	*(volatile uint8_t *)0xfe0004d0 = 0x20; /* 1 << 5 */
	*(volatile uint8_t *)0xfe0004d1 = 0x08; /* 1 << 11 */

	val = pcicfgread(usb12, 0x3c) &~ 0xffff;
	val = (('C' - '@') << 8) | 5;
	pcicfgwrite(usb12, 0x3c, val);
	val = pcicfgread(usb34, 0x3c) &~ 0xffff;
	val = (('C' - '@') << 8) | 5;
	pcicfgwrite(usb34, 0x3c, val);
	val = pcicfgread(ac97, 0x3c) &~ 0xffff;
	val = (('D' - '@') << 8) | 11;
	pcicfgwrite(ac97, 0x3c, val);
}

/*
 * WinBond/Symphony Lab 83C553 with PC87308 "SuperIO"
 *
 *	0.11.0	10ad.0565	PCI-ISA bridge
 *	0.11.1	10ad.0105	IDE (slide)
 */
void
setup_83C533F()
{
#if 0
	unsigned pcib, ide, val;

	pcib = pcimaketag(0, 11, 0);
	ide  = pcimaketag(0, 11, 1);
#endif
}

void
pcifixup()
{
	unsigned pcib, ide, nic, val, steer, irq;
	int line;

	switch (brdtype) {
	case BRD_SANDPOINTX3:
		pcib = pcimaketag(0, 11, 0);
		ide  = pcimaketag(0, 11, 1);
		nic  = pcimaketag(0, 15, 0);

		/*
		 * //// WinBond PIRQ ////
		 * 0x40 - bit 5 (0x20) indicates PIRQ presense
		 * 0x60 - PIRQ interrupt routing steer
		 */
		if (pcicfgread(pcib, 0x40) & 0x20) {
			steer = pcicfgread(pcib, 0x60);
			if ((steer & 0x80808080) == 0x80808080)
				printf("PIRQ[0-3] disabled\n");
			else {
				unsigned i, v = steer;
				for (i = 0; i < 4; i++, v >>= 8) {
					if ((v & 0x80) != 0 || (v & 0xf) == 0)
						continue;
					printf("PIRQ[%d]=%d\n", i, v & 0xf);
				}
			}
		}
		/*
		 * //// IDE fixup ////
		 *
		 * - "compatiblity mode" (ide 0x09)
		 * - IDE primary/secondary interrupt routing (pcib 0x43)
		 * - PCI interrupt routing (pcib 0x45/44)
		 * - no PCI pin/line assignment (ide 0x3d/3c)
		 */
		val = pcicfgread(ide, 0x08);
		val &= 0xffff00ff;
		pcicfgwrite(ide, 0x08, val | (0x8a << 8));

		/* 0x43 - IDE interrupt routing */
		val = pcicfgread(ide, 0x40) & 0x00ffffff;
		pcicfgwrite(ide, 0x40, val | (0xee << 24));

		/* 0x45/44 - PCI interrupt routing */
		val = pcicfgread(ide, 0x44) & 0xffff0000;
		pcicfgwrite(ide, 0x44, val);

		/* 0x3d/3c - turn off PCI pin/line */
		val = pcicfgread(ide, 0x3c) & 0xffff0000;
		pcicfgwrite(ide, 0x3c, val);

		/*
		 * //// fxp fixup ////
		 * - use PCI pin A line 15 (fxp 0x3d/3c)
		 */
		/* 0x3d/3c - PCI pin/line */
		val = pcicfgread(nic, 0x3c) & 0xffff0000;
		pcidecomposetag(nic, NULL, &line, NULL);
		val |= (('A' - '@') << 8) | line;
		pcicfgwrite(nic, 0x3c, val);
		break;

	case BRD_ENCOREPP1:
#define	STEER(v, b) (((v) & (b)) ? "edge" : "level")

		pcib = pcimaketag(0, 22, 0);
		ide  = pcimaketag(0, 22, 1);
		nic  = pcimaketag(0, 25, 0);

		/*
		 * //// VIA PIRQ ////
		 * 0x57/56/55/54 - Dx CB Ax xS
		 */
		val = pcicfgread(pcib, 0x54);	/* Dx CB Ax xs */
		steer = val & 0xf;
		irq = (val >> 12) & 0xf;	/* 15:12 */
		if (irq) {
			printf("pin A -> irq %d, %s\n",
				irq, STEER(steer, 0x1));
		}
		irq = (val >> 16) & 0xf;	/* 19:16 */
		if (irq) {
			printf("pin B -> irq %d, %s\n",
				irq, STEER(steer, 0x2));
		}
		irq = (val >> 20) & 0xf;	/* 23:20 */
		if (irq) {
			printf("pin C -> irq %d, %s\n",
				irq, STEER(steer, 0x4));
		}
		irq = (val >> 28);		/* 31:28 */
		if (irq) {
			printf("pin D -> irq %d, %s\n",
				irq, STEER(steer, 0x8));
		}

		/*
		 * //// IDE fixup ////
		 * - "compatiblity mode" (ide 0x09)
		 * - use primary only (ide 0x40)
		 * - ISA IRQ14/15 for primary/secondary (pcib 0x4a)
		 * - no PCI pin/line assignment (ide 0x3d/3c)
		 */
		/* 0x09 - interface; 'native' := 01 << (2 * chan) */
		val = pcicfgread(ide, 0x08);
		printf("vide 0x09: %02x\n", (val >> 8) & 0xff);
		val &= 0xffff00ff;
		pcicfgwrite(ide, 0x08, val | (0x8a << 8));

		/* 0x4a - IDE primary/secondary IRQ routing */
		val = pcicfgread(pcib, 0x48);
		printf("pcib 0x4a: %02x\n", (val >> 16) & 0xff);

		/* 0x3d/3c - turn off PCI pin/line */
		val = pcicfgread(ide, 0x3c) & 0xffff0000;
		pcicfgwrite(ide, 0x3c, val);

		/*
		 * //// fxp fixup ////
		 * - use PCI pin A line 25 (fxp 0x3d/3c)
		 */
		/* 0x3d/3c - PCI pin/line */
		val = pcicfgread(nic, 0x3c) & 0xffff0000;
		val |= (('A' - '@') << 8) | 25;
		pcicfgwrite(nic, 0x3c, val);
		break;
	}
}
