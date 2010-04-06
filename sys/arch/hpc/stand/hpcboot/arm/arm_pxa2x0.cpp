/*	$NetBSD: arm_pxa2x0.cpp,v 1.3 2010/04/06 16:20:28 nonaka Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arm/arm_arch.h>
#include <console.h>
#include <memory.h>
#include <arm/arm_pxa2x0.h>

/*
 * Intel XScale PXA 2x0
 */

#define	PAGE_SIZE		0x1000
#define	DRAM_BANK_NUM		4		/* total 256MByte */
#define	DRAM_BANK_SIZE		0x04000000	/* 64Mbyte */

#define	DRAM_BANK0_START	0xa0000000
#define	DRAM_BANK0_SIZE		DRAM_BANK_SIZE
#define	DRAM_BANK1_START	0xa4000000
#define	DRAM_BANK1_SIZE		DRAM_BANK_SIZE
#define	DRAM_BANK2_START	0xa8000000
#define	DRAM_BANK2_SIZE		DRAM_BANK_SIZE
#define	DRAM_BANK3_START	0xac000000
#define	DRAM_BANK3_SIZE		DRAM_BANK_SIZE
#define	ZERO_BANK_START		0xe0000000
#define	ZERO_BANK_SIZE		DRAM_BANK_SIZE

__BEGIN_DECLS

// 2nd bootloader
void boot_func_pxa2x0(kaddr_t, kaddr_t, kaddr_t, kaddr_t);
extern char boot_func_end_pxa2x0[];
#define	BOOT_FUNC_START		reinterpret_cast <vaddr_t>(boot_func_pxa2x0)
#define	BOOT_FUNC_END		reinterpret_cast <vaddr_t>(boot_func_end_pxa2x0)

/* jump to 2nd loader */
void FlatJump_pxa2x0(kaddr_t, kaddr_t, kaddr_t, kaddr_t);

__END_DECLS

PXA2X0Architecture::PXA2X0Architecture(Console *&cons, MemoryManager *&mem)
	: ARMArchitecture(cons, mem)
{
	DPRINTF((TEXT("PXA-2x0 CPU.\n")));
}

PXA2X0Architecture::~PXA2X0Architecture(void)
{
}

BOOL
PXA2X0Architecture::init(void)
{
	if (!_mem->init()) {
		DPRINTF((TEXT("can't initialize memory manager.\n")));
		return FALSE;
	}
	// set D-RAM information
	_mem->loadBank(DRAM_BANK0_START, DRAM_BANK_SIZE);
	_mem->loadBank(DRAM_BANK1_START, DRAM_BANK_SIZE);
	_mem->loadBank(DRAM_BANK2_START, DRAM_BANK_SIZE);
	_mem->loadBank(DRAM_BANK3_START, DRAM_BANK_SIZE);

	// set D-cache information
	dcachesize = 32768 * 2;
	DPRINTF((TEXT("D-cache size = %d\n"), dcachesize));

#ifdef HW_TEST
	DPRINTF((TEXT("Testing framebuffer.\n")));
	testFramebuffer();

	DPRINTF((TEXT("Testing UART.\n")));
	testUART();
#endif

#ifdef REGDUMP
	DPRINTF((TEXT("Dump peripheral registers.\n")));
	dumpPeripheralRegs();
#endif

#ifdef CS0DUMP
	uint32_t dumpsize = 1024 * 1024; // 1MB
	DPRINTF((TEXT("Dump CS area (size = %d)\n"), dumpsize));
	dumpCS0(dumpsize);
#endif

	return TRUE;
}

void
PXA2X0Architecture::testFramebuffer(void)
{
	DPRINTF((TEXT("No framebuffer test yet.\n")));
}

void
PXA2X0Architecture::testUART(void)
{
#define	COM_DATA		VOLATILE_REF8(uart + 0x00)
#define COM_IIR			VOLATILE_REF8(uart + 0x08)
#define	COM_LSR			VOLATILE_REF8(uart + 0x14)
#define LSR_TXRDY		0x20
#define	COM_TX_CHECK		while (!(COM_LSR & LSR_TXRDY))
#define	COM_PUTCHAR(c)		(COM_DATA = (c))
#define	COM_CLR_INTS		((void)COM_IIR)
#define	_(c)								\
__BEGIN_MACRO								\
	COM_TX_CHECK;							\
	COM_PUTCHAR(c);							\
	COM_TX_CHECK;							\
	COM_CLR_INTS;							\
__END_MACRO

	vaddr_t uart =
	    _mem->mapPhysicalPage(0x40100000, 0x100, PAGE_READWRITE);

	// Don't turn on the enable-UART bit in the IER; this seems to
	// result in WinCE losing the port (and nothing working later).
	// All that should be taken care of by using WinCE to open the
	// port before we actually use it.

	_('H');_('e');_('l');_('l');_('o');_(' ');
	_('W');_('o');_('r');_('l');_('d');_('\r');_('\n');

	_mem->unmapPhysicalPage(uart);
}

BOOL
PXA2X0Architecture::setupLoader(void)
{
	vaddr_t v;
	vsize_t sz = BOOT_FUNC_END - BOOT_FUNC_START;

	// check 2nd bootloader size.
	if (sz > _mem->getPageSize()) {
		DPRINTF((TEXT("2nd bootloader size(%dbyte) is larger than page size(%d).\n"),
		    sz, _mem->getPageSize()));
		return FALSE;
	}

	// get physical mapped page and copy loader to there.
	// don't writeback D-cache here. make sure to writeback before jump.
	if (!_mem->getPage(v , _loader_addr)) {
		DPRINTF((TEXT("can't get page for 2nd loader.\n")));
		return FALSE;
	}
	DPRINTF((TEXT("2nd bootloader vaddr=0x%08x paddr=0x%08x\n"),
	    (unsigned)v,(unsigned)_loader_addr));

	memcpy(reinterpret_cast <LPVOID>(v),
	    reinterpret_cast <LPVOID>(BOOT_FUNC_START), sz);
	DPRINTF((TEXT("2nd bootloader copy done.\n")));

	return TRUE;
}

void
PXA2X0Architecture::jump(paddr_t info, paddr_t pvec)
{
	kaddr_t sp;
	vaddr_t v;
	paddr_t p;

	// stack for bootloader 
	_mem->getPage(v, p);
	sp = ptokv(p) + _mem->getPageSize();
	DPRINTF((TEXT("sp for bootloader = %08x + %08x = %08x\n"),
	    ptokv(p), _mem->getPageSize(), sp));

	// writeback whole D-cache
	WritebackDCache();

	SetKMode(1);
	FlatJump_pxa2x0(info, pvec, sp, _loader_addr);
	// NOTREACHED
	SetKMode(0);
	DPRINTF((TEXT("Return from FlatJump_pxa2x0.\n")));
}


//
// dump CS0
//
void
PXA2X0Architecture::dumpCS0(uint32_t size)
{
	static char buf[0x1000];
	vaddr_t mem;
	uint32_t addr;
	uint32_t off;
	HANDLE fh;
	unsigned long wrote;

	fh = CreateFile(TEXT("rom.bin"), GENERIC_WRITE, FILE_SHARE_READ,
	    0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (fh == INVALID_HANDLE_VALUE) {
		DPRINTF((TEXT("can't open file. (%s)\n"), TEXT("rom.bin")));
		return;
	}

	for (addr = 0; addr < size; addr += 0x1000) {
		memset(buf, 0, sizeof(buf));
		mem = _mem->mapPhysicalPage(addr, 0x1000, PAGE_READWRITE);
		for (off = 0; off < 0x1000; off++) {
			buf[off] = VOLATILE_REF8(mem + off);
		}
		_mem->unmapPhysicalPage(mem);
		WriteFile(fh, buf, 0x1000, &wrote, 0);
	}

	CloseHandle(fh);
}

//
// dump peripheral registers.
//

#ifdef REGDUMP
#define	PXA250_GPIO_REG_NUM	3
#define	PXA250_GPIO_NUM		96
#define	PXA270_GPIO_REG_NUM	4
#define	PXA270_GPIO_NUM		121

static const TCHAR *pxa270_gpioName[PXA270_GPIO_NUM][7] =
{
/*0*/	{ TEXT("GPIO<0>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*1*/	{ TEXT("GPIO<1>/nRESET_GPIO"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*2*/	{ TEXT("SYS_EN5"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*3*/	{ TEXT("GPIO<3>/PWR_SCL"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*4*/	{ TEXT("GPIO<4>/PWR_SDA"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*5*/	{ TEXT("PWR_CAP<0>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*6*/	{ TEXT("PWR_CAP<1>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*7*/	{ TEXT("PWR_CAP<2>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*8*/	{ TEXT("PWR_CAP<3>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*9*/	{ TEXT("GPIO<9>"),TEXT(""),TEXT(""),TEXT("FFCTS"),TEXT("HZ_CLK"),TEXT(""),TEXT("CHOUT<0>"), },
/*10*/	{ TEXT("GPIO<10>"),TEXT("FFDCD"),TEXT(""),TEXT("USB_P3_57"),TEXT("HZ_CLK"),TEXT(""),TEXT("CHOUT<1>"), },
/*11*/	{ TEXT("GPIO<11>"),TEXT("EXT_SYNC<0>"),TEXT("SSPRXD2"),TEXT("USB_P3_1"),TEXT("CHOUT<0>"),TEXT("PWM_OUT<2>"),TEXT("48_MHz"), },
/*12*/	{ TEXT("GPIO<12>"),TEXT("EXT_SYNC<1>"),TEXT("CIF_DD<7>"),TEXT(""),TEXT("CHOUT<1>"),TEXT("PWM_OUT<3>"),TEXT("48_MHz"), },
/*13*/	{ TEXT("GPIO<13>"),TEXT("CLK_EXT"),TEXT("KP_DKIN<7>"),TEXT("KP_MKIN<7>"),TEXT("SSPTXD2"),TEXT(""),TEXT(""), },
/*14*/	{ TEXT("GPIO<14>"),TEXT("L_VSYNC"),TEXT("SSPSFRM2"),TEXT(""),TEXT(""),TEXT("SSPSFRM2"),TEXT("UCLK"), },
/*15*/	{ TEXT("GPIO<15>"),TEXT(""),TEXT(""),TEXT(""),TEXT("nPCE<1>"),TEXT("nCS<1>"),TEXT(""), },
/*16*/	{ TEXT("GPIO<16>"),TEXT("KP_MKIN<5>"),TEXT(""),TEXT(""),TEXT(""),TEXT("PWM_OUT<0>"),TEXT("FFTXD"), },
/*17*/	{ TEXT("GPIO<17>"),TEXT("KP_MKIN<6>"),TEXT("CIF_DD<6>"),TEXT(""),TEXT(""),TEXT("PWM_OUT<1>"),TEXT(""), },
/*18*/	{ TEXT("GPIO<18>"),TEXT("RDY"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*19*/	{ TEXT("GPIO<19>"),TEXT("SSPSCLK2"),TEXT(""),TEXT("FFRXD"),TEXT("SSPSCLK2"),TEXT("L_CS"),TEXT("nURST"), },
/*20*/	{ TEXT("GPIO<20>"),TEXT("DREQ<0>"),TEXT("MBREQ"),TEXT(""),TEXT("nSDCS<2>"),TEXT(""),TEXT(""), },
/*21*/	{ TEXT("GPIO<21>"),TEXT(""),TEXT(""),TEXT(""),TEXT("nSDCS<3>"),TEXT("DVAL<0>"),TEXT("MBGNT"), },
/*22*/	{ TEXT("GPIO<22>"),TEXT("SSPEXTCLK2"),TEXT("SSPSCLK2EN"),TEXT("SSPSCLK2"),TEXT("KP_MKOUT<7>"),TEXT("SSPSYSCLK2"),TEXT("SSPSCLK2"), },
/*23*/	{ TEXT("GPIO<23>"),TEXT(""),TEXT("SSPSCLK"),TEXT(""),TEXT("CIF_MCLK"),TEXT("SSPSCLK"),TEXT(""), },
/*24*/	{ TEXT("GPIO<24>"),TEXT("CIF_FV"),TEXT("SSPSFRM"),TEXT(""),TEXT("CIF_FV"),TEXT("SSPSFRM"),TEXT(""), },
/*25*/	{ TEXT("GPIO<25>"),TEXT("CIF_LV"),TEXT(""),TEXT(""),TEXT("CIF_LV"),TEXT("SSPTXD"),TEXT(""), },
/*26*/	{ TEXT("GPIO<26>"),TEXT("SSPRXD"),TEXT("CIF_PCLK"),TEXT("FFCTS"),TEXT(""),TEXT(""),TEXT(""), },
/*27*/	{ TEXT("GPIO<27>"),TEXT("SSPEXTCLK"),TEXT("SSPSCLKEN"),TEXT("CIF_DD<0>"),TEXT("SSPSYSCLK"),TEXT(""),TEXT("FFRTS"), },
/*28*/	{ TEXT("GPIO<28>"),TEXT("AC97_BITCLK"),TEXT("I2S_BITCLK"),TEXT("SSPSFRM"),TEXT("I2S_BITCLK"),TEXT(""),TEXT("SSPSFRM"), },
/*29*/	{ TEXT("GPIO<29>"),TEXT("AC97_SDATA_IN_0"),TEXT("I2S_SDATA_IN"),TEXT("SSPSCLK"),TEXT("SSPRXD2"),TEXT(""),TEXT("SSPSCLK"), },
/*30*/	{ TEXT("GPIO<30>"),TEXT(""),TEXT(""),TEXT(""),TEXT("I2S_SDATA_OUT"),TEXT("AC97_SDATA_OUT"),TEXT("USB_P3_2"), },
/*31*/	{ TEXT("GPIO<31>"),TEXT(""),TEXT(""),TEXT(""),TEXT("I2S_SYNC"),TEXT("AC97_SYNC"),TEXT("USB_P3_6"), },
/*32*/	{ TEXT("GPIO<32>"),TEXT(""),TEXT(""),TEXT(""),TEXT("MSSCLK"),TEXT("MMCLK"),TEXT(""), },
/*33*/	{ TEXT("GPIO<33>"),TEXT("FFRXD"),TEXT("FFDSR"),TEXT(""),TEXT("DVAL<1>"),TEXT("nCS<5>"),TEXT("MBGNT"), },
/*34*/	{ TEXT("GPIO<34>"),TEXT("FFRXD"),TEXT("KP_MKIN<3>"),TEXT("SSPSCLK3"),TEXT("USB_P2_2"),TEXT(""),TEXT("SSPSCLK3"), },
/*35*/	{ TEXT("GPIO<35>"),TEXT("FFCTS"),TEXT("USB_P2_1"),TEXT("SSPSFRM3"),TEXT(""),TEXT("KP_MKOUT<6>"),TEXT("SSPTXD3"), },
/*36*/	{ TEXT("GPIO<36>"),TEXT("FFDCD"),TEXT("SSPSCLK2"),TEXT("KP_MKIN<7>"),TEXT("USB_P2_4"),TEXT("SSPSCLK2"),TEXT(""), },
/*37*/	{ TEXT("GPIO<37>"),TEXT("FFDSR"),TEXT("SSPSFRM2"),TEXT("KP_MKIN<3>"),TEXT("USB_P2_8"),TEXT("SSPSFRM2"),TEXT("FFTXD"), },
/*38*/	{ TEXT("GPIO<38>"),TEXT("FFRI"),TEXT("KP_MKIN<4>"),TEXT("USB_P2_3"),TEXT("SSPTXD3"),TEXT("SSPTXD2"),TEXT("PWM_OUT<1>"), },
/*39*/	{ TEXT("GPIO<39>"),TEXT("KP_MKIN<4>"),TEXT(""),TEXT("SSPSFRM3"),TEXT("USB_P2_6"),TEXT("FFTXD"),TEXT("SSPSFRM3"), },
/*40*/	{ TEXT("GPIO<40>"),TEXT("SSPRXD2"),TEXT(""),TEXT("USB_P2_5"),TEXT("KP_MKOUT<6>"),TEXT("FFDTR"),TEXT("SSPSCLK3"), },
/*41*/	{ TEXT("GPIO<41>"),TEXT("FFRXD"),TEXT("USB_P2_7"),TEXT("SSPRXD3"),TEXT("KP_MKOUT<7>"),TEXT("FFRTS"),TEXT(""), },
/*42*/	{ TEXT("GPIO<42>"),TEXT("BTRXD"),TEXT("ICP_RXD"),TEXT(""),TEXT(""),TEXT(""),TEXT("CIF_MCLK"), },
/*43*/	{ TEXT("GPIO<43>"),TEXT(""),TEXT(""),TEXT("CIF_FV"),TEXT("ICP_TXD"),TEXT("BTTXD"),TEXT("CIF_FV"), },
/*44*/	{ TEXT("GPIO<44>"),TEXT("BTCTS"),TEXT(""),TEXT("CIF_LV"),TEXT(""),TEXT(""),TEXT("CIF_LV"), },
/*45*/	{ TEXT("GPIO<45>"),TEXT(""),TEXT(""),TEXT("CIF_PCLK"),TEXT("AC97_SYSCLK"),TEXT("BTRTS"),TEXT("SSPSYSCLK3"), },
/*46*/	{ TEXT("GPIO<46>"),TEXT("ICP_RXD"),TEXT("STD_RXD"),TEXT(""),TEXT(""),TEXT("PWM_OUT<2>"),TEXT(""), },
/*47*/	{ TEXT("GPIO<47>"),TEXT("CIF_DD<0>"),TEXT(""),TEXT(""),TEXT("STD_TXD"),TEXT("ICP_TXD"),TEXT("PWM_OUT<3>"), },
/*48*/	{ TEXT("GPIO<48>"),TEXT("CIF_DD<5>"),TEXT(""),TEXT(""),TEXT("BB_OB_DAT<1>"),TEXT("nPOE"),TEXT(""), },
/*49*/	{ TEXT("GPIO<49>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("nPWE"),TEXT(""), },
/*50*/	{ TEXT("GPIO<50>"),TEXT("CIF_DD<3>"),TEXT(""),TEXT("SSPSCLK2"),TEXT("BB_OB_DAT<2>"),TEXT("nPIOR"),TEXT("SSPSCLK2"), },
/*51*/	{ TEXT("GPIO<51>"),TEXT("CIF_DD<2>"),TEXT(""),TEXT(""),TEXT("BB_OB_DAT<3>"),TEXT("nPIOW"),TEXT(""), },
/*52*/	{ TEXT("GPIO<52>"),TEXT("CIF_DD<4>"),TEXT("SSPSCLK3"),TEXT(""),TEXT("BB_OB_CLK"),TEXT("SSPSCLK3"),TEXT(""), },
/*53*/	{ TEXT("GPIO<53>"),TEXT("FFRXD"),TEXT("USB_P2_3"),TEXT(""),TEXT("BB_OB_STB"),TEXT("CIF_MCLK"),TEXT("SSPSYSCLK"), },
/*54*/	{ TEXT("GPIO<54>"),TEXT(""),TEXT("BB_OB_WAIT"),TEXT("CIF_PCLK"),TEXT(""),TEXT("nPCE<2>"),TEXT(""), },
/*55*/	{ TEXT("GPIO<55>"),TEXT("CIF_DD<1>"),TEXT("BB_IB_DAT<1>"),TEXT(""),TEXT(""),TEXT("nPREG"),TEXT(""), },
/*56*/	{ TEXT("GPIO<56>"),TEXT("nPWAIT"),TEXT("BB_IB_DAT<2>"),TEXT(""),TEXT("USB_P3_4"),TEXT(""),TEXT(""), },
/*57*/	{ TEXT("GPIO<57>"),TEXT("nIOIS16"),TEXT("BB_IB_DAT<3>"),TEXT(""),TEXT(""),TEXT(""),TEXT("SSPTXD"), },
/*58*/	{ TEXT("GPIO<58>"),TEXT(""),TEXT("LDD<0>"),TEXT(""),TEXT(""),TEXT("LDD<0>"),TEXT(""), },
/*59*/	{ TEXT("GPIO<59>"),TEXT(""),TEXT("LDD<1>"),TEXT(""),TEXT(""),TEXT("LDD<1>"),TEXT(""), },
/*60*/	{ TEXT("GPIO<60>"),TEXT(""),TEXT("LDD<2>"),TEXT(""),TEXT(""),TEXT("LDD<2>"),TEXT(""), },
/*61*/	{ TEXT("GPIO<61>"),TEXT(""),TEXT("LDD<3>"),TEXT(""),TEXT(""),TEXT("LDD<3>"),TEXT(""), },
/*62*/	{ TEXT("GPIO<62>"),TEXT(""),TEXT("LDD<4>"),TEXT(""),TEXT(""),TEXT("LDD<4>"),TEXT(""), },
/*63*/	{ TEXT("GPIO<63>"),TEXT(""),TEXT("LDD<5>"),TEXT(""),TEXT(""),TEXT("LDD<5>"),TEXT(""), },
/*64*/	{ TEXT("GPIO<64>"),TEXT(""),TEXT("LDD<6>"),TEXT(""),TEXT(""),TEXT("LDD<6>"),TEXT(""), },
/*65*/	{ TEXT("GPIO<65>"),TEXT(""),TEXT("LDD<7>"),TEXT(""),TEXT(""),TEXT("LDD<7>"),TEXT(""), },
/*66*/	{ TEXT("GPIO<66>"),TEXT(""),TEXT("LDD<8>"),TEXT(""),TEXT(""),TEXT("LDD<8>"),TEXT(""), },
/*67*/	{ TEXT("GPIO<67>"),TEXT(""),TEXT("LDD<9>"),TEXT(""),TEXT(""),TEXT("LDD<9>"),TEXT(""), },
/*68*/	{ TEXT("GPIO<68>"),TEXT(""),TEXT("LDD<10>"),TEXT(""),TEXT(""),TEXT("LDD<10>"),TEXT(""), },
/*69*/	{ TEXT("GPIO<69>"),TEXT(""),TEXT("LDD<11>"),TEXT(""),TEXT(""),TEXT("LDD<11>"),TEXT(""), },
/*70*/	{ TEXT("GPIO<70>"),TEXT(""),TEXT("LDD<12>"),TEXT(""),TEXT(""),TEXT("LDD<12>"),TEXT(""), },
/*71*/	{ TEXT("GPIO<71>"),TEXT(""),TEXT("LDD<13>"),TEXT(""),TEXT(""),TEXT("LDD<13>"),TEXT(""), },
/*72*/	{ TEXT("GPIO<72>"),TEXT(""),TEXT("LDD<14>"),TEXT(""),TEXT(""),TEXT("LDD<14>"),TEXT(""), },
/*73*/	{ TEXT("GPIO<73>"),TEXT(""),TEXT("LDD<15>"),TEXT(""),TEXT(""),TEXT("LDD<15>"),TEXT(""), },
/*74*/	{ TEXT("GPIO<74>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("L_FCLK_RD"),TEXT(""), },
/*75*/	{ TEXT("GPIO<75>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("L_LCLK_A0"),TEXT(""), },
/*76*/	{ TEXT("GPIO<76>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("L_PCLK_WR"),TEXT(""), },
/*77*/	{ TEXT("GPIO<77>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("L_BIAS"),TEXT(""), },
/*78*/	{ TEXT("GPIO<78>"),TEXT(""),TEXT(""),TEXT(""),TEXT("nPCE<2>"),TEXT("nCS<2>"),TEXT(""), },
/*79*/	{ TEXT("GPIO<79>"),TEXT(""),TEXT(""),TEXT(""),TEXT("PSKTSEL"),TEXT("nCS<3>"),TEXT("PWM_OUT<2>"), },
/*80*/	{ TEXT("GPIO<80>"),TEXT("DREQ<1>"),TEXT("MBREQ"),TEXT(""),TEXT(""),TEXT("nCS<4>"),TEXT("PWM_OUT<3>"), },
/*81*/	{ TEXT("GPIO<81>"),TEXT(""),TEXT("CIF_DD<0>"),TEXT(""),TEXT("SSPTXD3"),TEXT("BB_OB_DAT<0>"),TEXT(""), },
/*82*/	{ TEXT("GPIO<82>"),TEXT("SSPRXD3"),TEXT("BB_IB_DAT<0>"),TEXT("CIF_DD<5>"),TEXT(""),TEXT(""),TEXT("FFDTR"), },
/*83*/	{ TEXT("GPIO<83>"),TEXT("SSPSFRM3"),TEXT("BB_IB_CLK"),TEXT("CIF_DD<4>"),TEXT("SSPSFRM3"),TEXT("FFTXD"),TEXT("FFRTS"), },
/*84*/	{ TEXT("GPIO<84>"),TEXT("SSPCLK3"),TEXT("BB_IB_STB"),TEXT("CIF_FV"),TEXT("SSPCLK3"),TEXT(""),TEXT("CIF_FV"), },
/*85*/	{ TEXT("GPIO<85>"),TEXT("FFRXD"),TEXT("DREQ<2>"),TEXT("CIF_LV"),TEXT("nPCE<1>"),TEXT("BB_IB_WAIT"),TEXT("CIF_LV"), },
/*86*/	{ TEXT("GPIO<86>"),TEXT("SSPRXD2"),TEXT("LDD<16>"),TEXT("USB_P3_5"),TEXT("nPCE<1>"),TEXT("LDD<16>"),TEXT(""), },
/*87*/	{ TEXT("GPIO<87>"),TEXT("nPCE<2>"),TEXT("LDD<17>"),TEXT("USB_P3_1"),TEXT("SSPTXD2"),TEXT("LDD<17>"),TEXT("SSPSFRM2"), },
/*88*/	{ TEXT("GPIO<88>"),TEXT("USBHPWR<1>"),TEXT("SSPRXD2"),TEXT("SSPSFRM2"),TEXT(""),TEXT(""),TEXT("SSPSFRM2"), },
/*89*/	{ TEXT("GPIO<89>"),TEXT("SSPRXD3"),TEXT(""),TEXT("FFRI"),TEXT("AC97_SYSCLK"),TEXT("USBHPEN<1>"),TEXT("SSPTXD2"), },
/*90*/	{ TEXT("GPIO<90>"),TEXT("KP_MKIN<5>"),TEXT("USB_P3_5"),TEXT("CIF_DD<4>"),TEXT(""),TEXT("nURST"),TEXT(""), },
/*91*/	{ TEXT("GPIO<91>"),TEXT("KP_MKIN<6>"),TEXT("USB_P3_1"),TEXT("CIF_DD<5>"),TEXT(""),TEXT("UCLK"),TEXT(""), },
/*92*/	{ TEXT("GPIO<92>"),TEXT("MMDAT<0>"),TEXT(""),TEXT(""),TEXT("MMDAT<0>"),TEXT("MSBS"),TEXT(""), },
/*93*/	{ TEXT("GPIO<93>"),TEXT("KP_DKIN<0>"),TEXT("CIF_DD<6>"),TEXT(""),TEXT("AC97_SDATA_OUT"),TEXT(""),TEXT(""), },
/*94*/	{ TEXT("GPIO<94>"),TEXT("KP_DKIN<1>"),TEXT("CIF_DD<5>"),TEXT(""),TEXT("AC97_SYNC"),TEXT(""),TEXT(""), },
/*95*/	{ TEXT("GPIO<95>"),TEXT("KP_DKIN<2>"),TEXT("CIF_DD<4>"),TEXT("KP_MKIN<6>"),TEXT("AC97_RESET_n"),TEXT(""),TEXT(""), },
/*96*/	{ TEXT("GPIO<96>"),TEXT("KP_DKIN<3>"),TEXT("MBREQ"),TEXT("FFRXD"),TEXT(""),TEXT("DVAL<1>"),TEXT("KP_MKOUT<6>"), },
/*97*/	{ TEXT("GPIO<97>"),TEXT("KP_DKIN<4>"),TEXT("DREQ<1>"),TEXT("KP_MKIN<3>"),TEXT(""),TEXT("MBGNT"),TEXT(""), },
/*98*/	{ TEXT("GPIO<98>"),TEXT("KP_DKIN<5>"),TEXT("CIF_DD<0>"),TEXT("KP_MKIN<4>"),TEXT("AC97_SYSCLK"),TEXT(""),TEXT("FFRTS"), },
/*99*/	{ TEXT("GPIO<99>"),TEXT("KP_DKIN<6>"),TEXT("AC97_SDATA_IN_1"),TEXT("KP_MKIN<5>"),TEXT(""),TEXT(""),TEXT("FFTXD"), },
/*100*/	{ TEXT("GPIO<100>"),TEXT("KP_MKIN<0>"),TEXT("DREQ<2>"),TEXT("FFCTS"),TEXT(""),TEXT(""),TEXT(""), },
/*101*/	{ TEXT("GPIO<101>"),TEXT("KP_MKIN<1>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*102*/	{ TEXT("GPIO<102>"),TEXT("KP_MKIN<2>"),TEXT(""),TEXT("FFRXD"),TEXT("nPCE<1>"),TEXT(""),TEXT(""), },
/*103*/	{ TEXT("GPIO<103>"),TEXT("CIF_DD<3>"),TEXT(""),TEXT(""),TEXT(""),TEXT("KP_MKOUT<0>"),TEXT(""), },
/*104*/	{ TEXT("GPIO<104>"),TEXT("CIF_DD<2>"),TEXT(""),TEXT(""),TEXT("PSKTSEL"),TEXT("KP_MKOUT<1>"),TEXT(""), },
/*105*/	{ TEXT("GPIO<105>"),TEXT("CIF_DD<1>"),TEXT(""),TEXT(""),TEXT("nPCE<2>"),TEXT("KP_MKOUT<2>"),TEXT(""), },
/*106*/	{ TEXT("GPIO<106>"),TEXT("CIF_DD<9>"),TEXT(""),TEXT(""),TEXT(""),TEXT("KP_MKOUT<3>"),TEXT(""), },
/*107*/	{ TEXT("GPIO<107>"),TEXT("CIF_DD<8>"),TEXT(""),TEXT(""),TEXT(""),TEXT("KP_MKOUT<4>"),TEXT(""), },
/*108*/	{ TEXT("GPIO<108>"),TEXT("CIF_DD<7>"),TEXT(""),TEXT(""),TEXT("CHOUT<0>"),TEXT("KP_MKOUT<5>"),TEXT(""), },
/*109*/	{ TEXT("GPIO<109>"),TEXT("MMDAT<1>"),TEXT("MSSDIO"),TEXT(""),TEXT("MMDAT<1>"),TEXT("MSSDIO"),TEXT(""), },
/*110*/	{ TEXT("GPIO<110>"),TEXT("MMDAT<2>/MMCCS<0>"),TEXT(""),TEXT(""),TEXT("MMDAT<2>/MMCCS<0>"),TEXT(""),TEXT(""), },
/*111*/	{ TEXT("GPIO<111>"),TEXT("MMDAT<3>/MMCCS<1>"),TEXT(""),TEXT(""),TEXT("MMDAT<3>/MMCCS<1>"),TEXT(""),TEXT(""), },
/*112*/	{ TEXT("GPIO<112>"),TEXT("MMCMD"),TEXT("nMSINS"),TEXT(""),TEXT("MMCMD"),TEXT(""),TEXT(""), },
/*113*/	{ TEXT("GPIO<113>"),TEXT(""),TEXT(""),TEXT("USB_P3_3"),TEXT("I2S_SYSCLK"),TEXT("AC97_RESET_n"),TEXT(""), },
/*114*/	{ TEXT("GPIO<114>"),TEXT("CIF_DD<1>"),TEXT(""),TEXT(""),TEXT("UEN"),TEXT("UVS0"),TEXT(""), },
/*115*/	{ TEXT("GPIO<115>"),TEXT("DREQ<0>"),TEXT("CIF_DD<3>"),TEXT("MBREQ"),TEXT("UEN"),TEXT("nUVS1"),TEXT("PWM_OUT<1>"), },
/*116*/	{ TEXT("GPIO<116>"),TEXT("CIF_DD<2>"),TEXT("AC97_SDATA_IN_0"),TEXT("UDET"),TEXT("DVAL<0>"),TEXT("nUVS2"),TEXT("MBGNT"), },
/*117*/	{ TEXT("GPIO<117>"),TEXT("SCL"),TEXT(""),TEXT(""),TEXT("SCL"),TEXT(""),TEXT(""), },
/*118*/	{ TEXT("GPIO<118>"),TEXT("SDA"),TEXT(""),TEXT(""),TEXT("SDA"),TEXT(""),TEXT(""), },
/*119*/	{ TEXT("GPIO<119>"),TEXT("USBHPWR<2>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
/*120*/	{ TEXT("GPIO<120>"),TEXT("USBHPEN<2>"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""), },
};
#endif

void
PXA2X0Architecture::dumpPeripheralRegs(void)
{
#ifdef REGDUMP
	uint32_t reg;
	bool first;
	int i, n;

#define	GPIO_OFFSET(r,o) ((r == 3) ? (o + 0x100) : (o + (r * 4)))
	// GPIO
	if (platid_match(&platid, &platid_mask_CPU_ARM_XSCALE_PXA270))
		n = PXA270_GPIO_REG_NUM;
	else
		n = PXA250_GPIO_REG_NUM;

	vaddr_t gpio =
	    _mem->mapPhysicalPage(0x40e00000, 0x1000, PAGE_READWRITE);
	DPRINTF((TEXT("Dump GPIO registers.\n")));
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x00));
		DPRINTF((TEXT("GPLR%d: 0x%08x\n"), i, reg));
	}
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x0c));
		DPRINTF((TEXT("GPDR%d: 0x%08x\n"), i, reg));
	}
#if 0	/* write-only register */
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x18));
		DPRINTF((TEXT("GPSR%d: 0x%08x\n"), reg));
	}
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x24));
		DPRINTF((TEXT("GPCR%d: 0x%08x\n"), reg));
	}
#endif
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x30));
		DPRINTF((TEXT("GRER%d: 0x%08x\n"), i, reg));
	}
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x3c));
		DPRINTF((TEXT("GFER%d: 0x%08x\n"), i, reg));
	}
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x48));
		DPRINTF((TEXT("GEDR%d: 0x%08x\n"), i, reg));
	}
	for (i = 0; i < n; i++) {
		reg = VOLATILE_REF(gpio + 0x54 + (i * 8));
		DPRINTF((TEXT("GAFR%d_L: 0x%08x\n"), i, reg));
		reg = VOLATILE_REF(gpio + 0x58 + (i * 8));
		DPRINTF((TEXT("GAFR%d_U: 0x%08x\n"), i, reg));
	}

	//
	// display detail
	//

	if (!platid_match(&platid, &platid_mask_CPU_ARM_XSCALE_PXA270))
		return;

	// header
	DPRINTF((TEXT("pin#,function,name,rising,falling,status\n")));

	n = PXA270_GPIO_NUM;
	for (i = 0; i < n; i++) {
		const TCHAR *fn_name, *pin_name;
		uint32_t dir, altfn, redge, fedge, status;

		// pin function
		dir = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x0c));
		dir = (dir >> (i % 32)) & 1;
		altfn = VOLATILE_REF(gpio + 0x54 + ((i / 16) * 4));
		altfn = (altfn >> ((i % 16) * 2)) & 3;
		if (altfn == 0) {
			if (dir == 0) {
				fn_name = TEXT("GPIO_IN");
			} else {
				fn_name = TEXT("GPIO_OUT");
			}
			DPRINTF((TEXT("%d,%s,%s,"), i, fn_name,
			    pxa270_gpioName[i][0]));
		} else {
			if (dir == 0) {
				fn_name = TEXT("IN");
				pin_name = pxa270_gpioName[i][altfn];
			} else {
				fn_name = TEXT("OUT");
				pin_name = pxa270_gpioName[i][altfn+3];
			}
			DPRINTF((TEXT("%d,ALT_FN_%d_%s,%s,"), i, altfn,
			    fn_name, pin_name));
		}

		// edge detect
		redge = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x30));
		redge = (redge >> (i % 32)) & 1;
		fedge = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x3c));
		fedge = (fedge >> (i % 32)) & 1;
		DPRINTF((TEXT("%s,%s,"),
		    redge ? TEXT("enable") : TEXT("disable"),
		    fedge ? TEXT("enable") : TEXT("disable")));

		// status
		status = VOLATILE_REF(gpio + GPIO_OFFSET(i, 0x00));
		status = (status >> (i % 32)) & 1;
		DPRINTF((TEXT("%s"), status ? TEXT("high") : TEXT("low")));

		DPRINTF((TEXT("\n")));
	}
	_mem->unmapPhysicalPage(gpio);

	// LCDC
	DPRINTF((TEXT("Dump LCDC registers.\n")));
	vaddr_t lcdc =
	    _mem->mapPhysicalPage(0x44000000, 0x1000, PAGE_READWRITE);
	reg = VOLATILE_REF(lcdc + 0x00);
	DPRINTF((TEXT("LCCR0: 0x%08x\n"), reg));
	DPRINTF((TEXT("-> ")));
	first = true;
	if (reg & (1U << 26)) {
		DPRINTF((TEXT("%sLDDALT"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 25)) {
		DPRINTF((TEXT("%sOUC"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 22)) {
		DPRINTF((TEXT("%sLCDT"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 9)) {
		DPRINTF((TEXT("%sDPD"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 7)) {
		DPRINTF((TEXT("%sPAS"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 2)) {
		DPRINTF((TEXT("%sSDS"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 1)) {
		DPRINTF((TEXT("%sCMS"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 0)) {
		DPRINTF((TEXT("%sENB"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	DPRINTF((TEXT("\n")));
	DPRINTF((TEXT("-> PDD = 0x%02x\n"), (reg >> 12) & 0xff));
	reg = VOLATILE_REF(lcdc + 0x04);
	DPRINTF((TEXT("LCCR1: 0x%08x\n"), reg));
	DPRINTF((TEXT("-> BLW = 0x%02x, ELW = 0x%02x, HSW = 0x%02x, PPL = 0x%03x\n"),
	    (reg >> 24) & 0xff, (reg >> 16) & 0xff, (reg >> 10) & 0x3f,
	    reg & 0x3ff));
	reg = VOLATILE_REF(lcdc + 0x08);
	DPRINTF((TEXT("LCCR2: 0x%08x\n"), reg));
	DPRINTF((TEXT("-> BFW = 0x%02x, EFW = 0x%02x, VSW = 0x%02x, LPP = 0x%03x\n"),
	    (reg >> 24) & 0xff, (reg >> 16) & 0xff, (reg >> 10) & 0x3f,
	    reg & 0x3ff));
	reg = VOLATILE_REF(lcdc + 0x0c);
	DPRINTF((TEXT("LCCR3: 0x%08x\n"), reg));
	DPRINTF((TEXT("-> ")));
	first = true;
	if (reg & (1U << 27)) {
		DPRINTF((TEXT("%sDPC"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 23)) {
		DPRINTF((TEXT("%sOEP"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 22)) {
		DPRINTF((TEXT("%sPCP"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 21)) {
		DPRINTF((TEXT("%sHSP"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 20)) {
		DPRINTF((TEXT("%sVSP"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	if (reg & (1U << 19)) {
		DPRINTF((TEXT("%sVSP"), first ? TEXT("") : TEXT("/")));
		first = false;
	}
	DPRINTF((TEXT("\n")));
	DPRINTF((TEXT("-> PDFOR = %d\n"), (reg >> 30) & 3));
	DPRINTF((TEXT("-> BPP = 0x%02x\n"), ((reg >> 29) & 1 << 3) | ((reg >> 24) & 7)));
	DPRINTF((TEXT("-> API = 0x%x\n"), (reg >> 16) & 0xf));
	DPRINTF((TEXT("-> ACB = 0x%02x\n"), (reg >> 8) & 0xff));
	DPRINTF((TEXT("-> PCD = 0x%02x\n"), reg & 0xff));
	reg = VOLATILE_REF(lcdc + 0x10);
	DPRINTF((TEXT("LCCR4: 0x%08x\n"), reg));
	DPRINTF((TEXT("-> PCDDIV = %d\n"), (reg >> 31) & 1));
	DPRINTF((TEXT("-> PAL_FOR = %d\n"), (reg >> 15) & 3));
	DPRINTF((TEXT("-> K3 = 0x%x, K2 = 0x%x, K1 = 0x%x\n"),
	    (reg >> 6) & 7, (reg >> 3) & 7, reg & 7));
	reg = VOLATILE_REF(lcdc + 0x14);
	DPRINTF((TEXT("LCCR5: 0x%08x\n"), reg));
	reg = VOLATILE_REF(lcdc + 0x54);
	DPRINTF((TEXT("LCDBSCNTR: 0x%08x\n"), reg));
	_mem->unmapPhysicalPage(lcdc);
#endif
}
