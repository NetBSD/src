/*	$NetBSD: disptest.c,v 1.2 1999/09/26 02:42:50 takemura Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <pbsdboot.h>

#define ARRAYSIZEOF(a)	(sizeof(a)/sizeof(*(a)))

static struct area {
  long start, end;
} targets[] = {
	{ 0x0a000000, 0x0b000000 },
	{ 0x10000000, 0x14000000 },
};
int ntargets = ARRAYSIZEOF(targets);

void
flush_XX()
{
  static volatile unsigned char tmp[1024*64];
  int i, s;

  for (i = 0; i < ARRAYSIZEOF(tmp); i++) {
	  s += tmp[i];
  }
}

static void
gpio_test()
{
#define GIUBASE 0xab000000
#define GIUOFFSET 0x0100
	volatile unsigned short *giusell;
	volatile unsigned short *giuselh;
	volatile unsigned short *giupiodl;
	volatile unsigned short *giupiodh;
	unsigned short sell = 0;
	unsigned short selh = 0;
	unsigned short piodl = 0;
	unsigned short piodh = 0;
	int res, i;
	unsigned short regs[16];
	unsigned short prev_regs[16];

	unsigned char* p = (char*)VirtualAlloc(0, 1024, MEM_RESERVE,
				      PAGE_NOACCESS);
	res = VirtualCopy((LPVOID)p, (LPVOID)(GIUBASE >> 8), 1024,
			  PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	if (!res) {
		win_printf(TEXT("VirtualCopy() failed."));
	}

	for (i = 0; i < 16; i++) {
		prev_regs[i] = ~0;
	}

	giusell = (unsigned short*)(p + GIUOFFSET + 0);
	giuselh = (unsigned short*)(p + GIUOFFSET + 2);
	giupiodl = (unsigned short*)(p + GIUOFFSET + 4);
	giupiodh = (unsigned short*)(p + GIUOFFSET + 6);

	while (1) {
		sell = *giusell;
		selh = *giuselh;
		*giusell = sell;
		*giuselh = selh;

		piodl = *giupiodl;
		piodh = *giupiodh;
		*giupiodl = piodl;
		*giupiodh = piodh;
		for (i = 0; i < 16; i++) {
			regs[i] = *(unsigned short*)(p + GIUOFFSET + i * 2);
		}
		for (i = 0; i < 16; i++) {
			if (i != 3 && i != 5 && regs[i] != prev_regs[i]) {
				win_printf(TEXT("IOSEL=%04x%04x "),
					   regs[1], regs[0]);
				win_printf(TEXT("PIOD=%04x%04x "),
					   regs[3], regs[2]);
				win_printf(TEXT("STAT=%04x%04x "),
					   regs[5], regs[4]);
				win_printf(TEXT("(%04x%04x) "),
					   regs[5]&regs[7], regs[4]&regs[6]);
				win_printf(TEXT("EN=%04x%04x "),
					   regs[7], regs[6]);
				win_printf(TEXT("TYP=%04x%04x "),
					   regs[9], regs[8]);
				win_printf(TEXT("ALSEL=%04x%04x "),
					   regs[11], regs[10]);
				win_printf(TEXT("HTSEL=%04x%04x "),
					   regs[13], regs[12]);
				win_printf(TEXT("PODAT=%04x%04x "),
					   regs[15], regs[14]);
				win_printf(TEXT("\n"));
				for (i = 0; i < 16; i++) {
					prev_regs[i] = regs[i];
				}
				break;
			}
		}
	}
}

static void
serial_test()
{
#if 1
#  define SIUADDR 0xac000000
#  define REGOFFSET 0x0
#else
#  define SIUADDR 0xab000000
#  define REGOFFSET 0x1a0
#endif
#define REGSIZE 32
	int i, changed, res;
	unsigned char regs[REGSIZE], prev_regs[REGSIZE];
	unsigned char* p = (char*)VirtualAlloc(0, 1024, MEM_RESERVE,
				      PAGE_NOACCESS);
	
	for (i = 0; i < ARRAYSIZEOF(prev_regs); i++) {
		prev_regs[i] = ~0;
	}

	res = VirtualCopy((LPVOID)p, (LPVOID)(SIUADDR >> 8), 1024,
			  PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	if (!res) {
		win_printf(TEXT("VirtualCopy() failed."));
	}

	while (1) {
		flush_XX();

		for (i = 0; i < ARRAYSIZEOF(regs); i++) {
			regs[i] = p[REGOFFSET + i];
		}

		changed = 0;
		for (i = 0; i < ARRAYSIZEOF(regs); i++) {
			if (regs[i] != prev_regs[i]) {
				changed++;
			}
			prev_regs[i] = regs[i];
		}
		if (changed) {
			win_printf(TEXT("SIU regs: "));
			for (i = 0; i < ARRAYSIZEOF(regs); i++) {
				win_printf(TEXT("%02x "), regs[i]);
			}
			win_printf(TEXT("\n"));
		}
	}

	VirtualFree(p, 0, MEM_RELEASE);
}

static long
checksum(char* addr, int size)
{
	long sum = 0;
	int i;

	for (i = 0; i < size; i++) {
		sum += *addr++ * i;
	}
	return (sum);
}

static int
examine(char* addr, int size)
{
	long random_data[256];
	long dijest;
	int i;

	for (i = 0; i < ARRAYSIZEOF(random_data); i++) {
		random_data[i] = Random();
	}
	if (sizeof(random_data) < size) {
		size = sizeof(random_data);
	}
	memcpy(addr, (char*)random_data, size);
	dijest= checksum((char*)random_data, size);

	return (dijest == checksum(addr, size));
}

void
display_search()
{
	int step = 0x10000;
	int i;
	long addr;

	for (i = 0; i < ntargets; i++) {
		int prevres = -1;
		for (addr = targets[i].start;
		     addr < targets[i].end;
		     addr += step) {
			int res;
			char* p = (char*)VirtualAlloc(0, step, MEM_RESERVE,
						      PAGE_NOACCESS);
			res = VirtualCopy((LPVOID)p, (LPVOID)(addr >> 8), step,
				   PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
			if (!res) {
				win_printf(TEXT("VirtualCopy() failed."));
			}
			res = examine(p, step);
			VirtualFree(p, 0, MEM_RELEASE);
			if (res != prevres && prevres != -1) {
				if (res) {
					win_printf(TEXT("0x%x "), addr);
				} else {
					win_printf(TEXT("- 0x%x\n"), addr);
				}
			} else
			if (res && prevres == -1) {
				win_printf(TEXT("0x%x "), addr);
			}
			prevres = res;
		}
		if (prevres) {
			win_printf(TEXT("\n"));
		}
	}
}

void
display_draw()
{
	long addr = 0x13000000;
	int size = 0x40000;
	char* p;
	int i, j, res;
	int x, y;

	p = (char*)VirtualAlloc(0, size, MEM_RESERVE,
				PAGE_NOACCESS);
	res = VirtualCopy((LPVOID)p, (LPVOID)(addr >> 8), size,
			  PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	if (!res) {
		win_printf(TEXT("VirtualCopy() failed."));
	}

	for (i = 0; i < 10000; i++) {
		p[i] = i;
	}
	for (x = 0; x < 640; x += 10) {
		for (y = 0; y < 240; y += 1) {
		        p[1024 * y + x] = (char)0xff;
		}
	}
	for (y = 0; y < 240; y += 10) {
		for (x = 0; x < 640; x += 1) {
		        p[1024 * y + x] = (char)0xff;
		}
	}
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 16; j++) {
			for (x = i * 32; x < i * 32 + 32; x++) {
				for (y = j * 15; y < j * 15 + 15; y++) {
					p[1024 * y + x] = j * 16 + i;
				}
			}
		}
	}

	VirtualFree(p, 0, MEM_RELEASE);
}

#define PCIC_IDENT		0x00
#define	PCIC_REG_INDEX		0
#define	PCIC_REG_DATA		1
#define PCIC_IDENT_EXPECTED	0x83

void
pcic_search()
{
	long addr;
	int window_size = 0x10000;
	int i;

	for (addr = 0x14000000; addr < 0x18000000; addr += window_size) {
		int res;
		unsigned char* p;
		p = (char*)VirtualAlloc(0, window_size, MEM_RESERVE,
					PAGE_NOACCESS);
		res = VirtualCopy((LPVOID)p, (LPVOID)(addr >> 8), window_size,
				  PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
		if (!res) {
			win_printf(TEXT("VirtualCopy() failed."));
		}

		for (i = 0; i < window_size; i += 2) {
			p[i + PCIC_REG_INDEX] = PCIC_IDENT;
			if (p[i + PCIC_REG_DATA] == PCIC_IDENT_EXPECTED) {
				win_printf(TEXT("pcic is found at 0x%x\n"),
					   addr + i);
			}
		}

		VirtualFree(p, 0, MEM_RELEASE);
	}
}

void
hardware_test()
{
	int do_gpio_test = 0;
	int do_serial_test = 0;
	int do_display_draw = 0;
	int do_display_search = 0;
	int do_pcic_search = 0;

	if (do_gpio_test) {
		gpio_test();
	}
	if (do_serial_test) {
		serial_test();
	}
	if (do_display_draw) {
		display_draw();
	}
	if (do_display_search) {
		display_search();
	}
	if (do_pcic_search) {
		pcic_search();
	}
}
