/*	$NetBSD: switch.c,v 1.2.4.2 2014/08/20 00:03:28 tls Exp $	*/

/*
 * Copyright (c) 2014 Tetsuya Isaki. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "switch.h"

#define SRAM_MEMSIZE	(*((volatile uint32_t *)0x00ed0008))
#define SRAM_ROMADDR	(*((volatile uint32_t *)0x00ed000c))
#define SRAM_RAMADDR	(*((volatile uint32_t *)0x00ed0010))
#define SRAM_BOOTDEV	(*((volatile uint16_t *)0x00ed0018))

#define SYSPORT_SRAM_WP	(*((volatile uint8_t *)0x00e8e00d))

static int hextoi(const char *);
static void cmd_switch_help(void);
static void cmd_switch_show(void);
static void cmd_switch_show_boot(void);
static void cmd_switch_show_rom(void);
static void cmd_switch_show_memory(void);
static const char *romaddr_tostr(uint32_t);
static const char *get_romaddr_name(uint32_t);
static void cmd_switch_boot(const char *);
static void cmd_switch_rom(const char *);
static void cmd_switch_memory(const char *);

static inline void
sram_write_enable(void)
{
	SYSPORT_SRAM_WP = 0x31;
}

static inline void
sram_write_disable(void)
{
	SYSPORT_SRAM_WP = 0;
}

static int
hextoi(const char *in)
{
	char *c;
	int ret;

	ret = 0;
	c = (char *)in;
	for (; isxdigit(*c); c++) {
		ret = (ret * 16) +
		      (*c > '9' ? ((*c | 0x20) - 'a' + 10) : *c - '0');
	}
	return ret;
}

static void
cmd_switch_help(void)
{
	printf(
		"usage: switch <key>=<val>\n"
		"         boot=[std | inscsi<N> | exscsi<N> | fd<N> | rom ]\n"
		"         rom=[ inscsi<N> | exscsi<N> | $<addr> ]\n"
		"         memory=<1..12> (unit:MB)\n"
		"       switch show\n"
	);
}

void
cmd_switch(char *arg)
{
	char *val;

	if (strcmp(arg, "show") == 0) {
		cmd_switch_show();
		return;
	}

	val = strchr(arg, '=');
	if (val == NULL) {
		cmd_switch_help();
		return;
	}
	*val++ = '\0';

	if (strcmp(arg, "boot") == 0) {
		cmd_switch_boot(val);
	} else if (strcmp(arg, "rom") == 0) {
		cmd_switch_rom(val);
	} else if (strcmp(arg, "memory") == 0) {
		cmd_switch_memory(val);
	} else {
		cmd_switch_help();
	}
}

static void
cmd_switch_show(void)
{
	cmd_switch_show_boot();
	cmd_switch_show_rom();
	cmd_switch_show_memory();
}

static void
cmd_switch_show_boot(void)
{
	uint32_t romaddr;
	uint16_t bootdev;
	const char *name;

	bootdev = SRAM_BOOTDEV;
	romaddr = SRAM_ROMADDR;

	/*
	 * $0000: std
	 * $8n00: sasi<N>
	 * $9n70: fd<N>
	 * $a000: ROM
	 * $b000: RAM
	 */
	printf("boot=");
	switch (bootdev >> 12) {
	default:
	case 0x0:
		/*
		 * The real order is fd->sasi->rom->ram
		 * but it is a bit redundant..
		 */
		printf("std (fd -> ");
		name = get_romaddr_name(romaddr);
		if (name)
			printf("%s)", name);
		else
			printf("rom$%x)", romaddr);
		break;
	case 0x8:
		printf("sasi%d", (bootdev >> 8) & 15);
		break;
	case 0x9:
		printf("fd%d", (bootdev >> 8) & 3);
		break;
	case 0xa:
		printf("rom%s", romaddr_tostr(romaddr));
		break;
	case 0xb:
		printf("ram$%x", SRAM_RAMADDR);
		break;
	}
	printf("\n");
}

static void
cmd_switch_show_rom(void)
{
	uint32_t romaddr;

	romaddr = SRAM_ROMADDR;
	printf("rom=%s\n", romaddr_tostr(romaddr));
}

static void
cmd_switch_show_memory(void)
{
	printf("memory=%d MB\n", SRAM_MEMSIZE / (1024 * 1024));
}

/* return rom address as string with name if any */
static const char *
romaddr_tostr(uint32_t addr)
{
	static char buf[32];
	const char *name;

	name = get_romaddr_name(addr);
	if (name)
		snprintf(buf, sizeof(buf), "$%x (%s)", addr, name);
	else
		snprintf(buf, sizeof(buf), "$%x", addr);

	return buf;
}

/*
 * return "inscsiN" / "exscsiN" if addr is in range of SCSI boot.
 * Otherwise return NULL.
 */
static const char *
get_romaddr_name(uint32_t addr)
{
	static char buf[8];

	if (0xfc0000 <= addr && addr < 0xfc0020 && addr % 4 == 0) {
		snprintf(buf, sizeof(buf), "inscsi%d", (addr >> 2) & 7);
	} else if (0xea0020 <= addr && addr < 0xea0040 && addr % 4 == 0) {
		snprintf(buf, sizeof(buf), "exscsi%d", (addr >> 2) & 7);
	} else {
		return NULL;
	}
	return buf;
}

static void
cmd_switch_boot(const char *arg)
{
	int id;
	uint32_t romaddr;
	uint16_t bootdev;

	romaddr = 0xffffffff;

	if (strcmp(arg, "std") == 0) {
		bootdev = 0x0000;

	} else if (strcmp(arg, "rom") == 0) {
		bootdev = 0xa000;

	} else if (strncmp(arg, "inscsi", 6) == 0) {
		id = (arg[6] - '0') & 7;
		bootdev = 0xa000;
		romaddr = 0xfc0000 + id * 4;

	} else if (strncmp(arg, "exscsi", 6) == 0) {
		id = (arg[6] - '0') & 7;
		bootdev = 0xa000;
		romaddr = 0xea0020 + id * 4;

	} else if (strncmp(arg, "fd", 2) == 0) {
		id = (arg[2] - '0') & 3;
		bootdev = 0x9070 | (id << 8);

	} else {
		cmd_switch_help();
		return;
	}

	sram_write_enable();
	SRAM_BOOTDEV = bootdev;
	if (romaddr != 0xffffffff)
		SRAM_ROMADDR = romaddr;
	sram_write_disable();

	cmd_switch_show_boot();
}

static void
cmd_switch_rom(const char *arg)
{
	int id;
	uint32_t romaddr;

	if (strncmp(arg, "inscsi", 6) == 0) {
		id = (arg[6] - '0') & 7;
		romaddr = 0xfc0000 + id * 4;

	} else if (strncmp(arg, "exscsi", 6) == 0) {
		id = (arg[6] - '0') & 7;
		romaddr = 0xea0020 + id * 4;

	} else if (*arg == '$') {
		romaddr = hextoi(arg + 1);

	} else {
		cmd_switch_help();
		return;
	}

	sram_write_enable();
	SRAM_ROMADDR = romaddr;
	sram_write_disable();

	cmd_switch_show_rom();
}

static void
cmd_switch_memory(const char *arg)
{
	int num;

	num = atoi(arg);
	if (num < 1 || num > 12) {
		cmd_switch_help();
		return;
	}

	sram_write_enable();
	SRAM_MEMSIZE = num * (1024 * 1024);
	sram_write_disable();

	cmd_switch_show_memory();
}
