/*	$NetBSD: properties.c,v 1.1.1.1 1999/06/21 15:56:03 minoura Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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


#include <sys/types.h>

#include "memswitch.h"
#include "methods.h"


/*
 * XXX: gcc extension is used.
 */
struct property properties[] = {
	{
		"special", "magic1",
		0, 4, 0, {longword:0}, 0, {longword:0}, {longword:MAGIC1},
		parse_dummy, 0, 0,
		print_magic,
		fill_ulong, flush_dummy,
		" Magic.  Must be 0xa3d83638\n"
	},
	{
		"special", "magic2",
		4, 4, 0, {longword:0}, 0, {longword:0}, {longword:MAGIC2},
		parse_dummy, 0, 0,
		print_magic,
		fill_ulong, flush_dummy,
		" Magic.  Must be 0x30303057\n"
	},
	{
		"alarm", "bootmode",
		30, 4, 0, {longword:0}, 0, {longword:0}, {longword:0},
		parse_ulong, 0, 0xff0000,
		print_ulongh,
		fill_ulong, flush_ulong,
		" What to do on RTC alarm boot.\n"
	},
	{
		"alarm", "boottime",
		34, 4, 0, {longword:0}, 0, {longword:0}, {longword:0xffff0000},
		parse_ulong, 0, 0xffffffff,
		print_ulongh,
		fill_ulong, flush_ulong,
		" Alarm.\n"
	},
	{
		"alarm", "enabled",
		38, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 7,
		print_uchar,
		fill_uchar, flush_uchar,
		" 0 to enable alarm, 7 to disable.\n"
	},
	{
		"alarm", "timetodown",
		20, 4, 0, {longword:0}, 0, {longword:0}, {longword:-1},
		parse_time, -1, 0x7fffffff,
		print_timesec,
		fill_ulong, flush_ulong,
		" When boot on alarm, time to shutdown is stored in second.\n"
		" Can be specified in minite with suffix minute.\n"
	},
	{
		"boot", "device",
		24, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] -1}},
		parse_bootdev, 0, 0,
		print_bootdev,
		fill_ushort, flush_ushort,
		" Boot device.\n"
		" STD for standard, HDn for the nth harddisk,  FDn for the nth floppy drive,\n"
		" ROM for ROM, RAM for RAM.\n"
	},
	{
		"boot", "ramaddr",
		16, 4, 0, {longword:0}, 0, {longword:0}, {longword:0xed0100},
		parse_ulong, 0xed0000, 0xed3fff,
		print_ulongh,
		fill_ulong, flush_ulong,
		" If boot.device specifies to boot from RAM, the start address is stored.\n"
	},
	{
		"boot", "romaddr",
		12, 4, 0, {longword:0}, 0, {longword:0}, {longword:0xbffffc},
		parse_ulong, 0xe80000, 0xffffff,
		print_ulongh,
		fill_ulong, flush_ulong,
		" If boot.device specifies to boot from ROM, the start address is stored.\n"
	},
	{
		"display", "contrast",
		40, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 14}},
		parse_uchar, 0, 1,
		print_uchar,
		fill_uchar, flush_uchar,
		" Display contrast (0-15).\n"
	},
	{
		"display", "dentakufont",
		44, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 1,
		print_uchar,
		fill_uchar, flush_uchar,
		" In-line calculator font.  0 for LCD-like, 1 for normal.\n"
		" Note on NetBSD in-line calculator is not supported.\n"
	},
	{
		"display", "glyphmode",
		89, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 0x7,
		print_ucharh,
		fill_uchar, flush_uchar,
		" Glyph mode for ASCII/JIS ROMAN characters (bitmap).\n"
		" Bit 0 (LSB) is for codepoint 0x5c (\\), bit 1 for 0x7e (~),\n"
		" bit 2 for 0x7c (|).\n"
		" 0 for JIS ROMAN, 1 for ASCII.\n"
	},
	{
		"display", "resolution",
		29, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 16}},
		parse_uchar, 0, 18,
		print_ucharh,
		fill_uchar, flush_uchar,
		" Initial display resolution.\n"
	},
	{
		"display", "tcolor0",
		46, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] 0}},
		parse_ushort, 0, 0xffff,
		print_ushorth,
		fill_ushort, flush_ushort,
		" Initial RGB value for color cell #0.\n"
		" Note on NetBSD the value is ignored.\n"
	},
	{
		"display", "tcolor1",
		48, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] 0xf83e}},
		parse_ushort, 0, 0xffff,
		print_ushorth,
		fill_ushort, flush_ushort,
		" Initial RGB value for color cell #1.\n"
		" Note on NetBSD the value is ignored.\n"
	},
	{
		"display", "tcolor2",
		50, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] 0xffc0}},
		parse_ushort, 0, 0xffff,
		print_ushorth,
		fill_ushort, flush_ushort,
		" Initial RGB value for color cell #2.\n"
		" Note on NetBSD the value is ignored.\n"
	},
	{
		"display", "tcolor3",
		52, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] 0xfffe}},
		parse_ushort, 0, 0xffff,
		print_ushorth,
		fill_ushort, flush_ushort,
		" Initial RGB value for color cell #3.\n"
		" Note on NetBSD the value is ignored.\n"
	},
	{
		"display", "tcolor47",
		54, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] 0xde6c}},
		parse_ushort, 0, 0xffff,
		print_ushorth,
		fill_ushort, flush_ushort,
		" Initial RGB value for color cell #4-7.\n"
		" Note on NetBSD the value is ignored.\n"
	},
	{
		"display", "tcolor8f",
		56, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] 0}},
		parse_ushort, 0, 0xffff,
		print_ushorth,
		fill_ushort, flush_ushort,
		" Initial RGB value for color cell #8-15.\n"
		" Note on NetBSD the value is ignored.\n"
	},
	{
		"hw", "harddrive",
		90, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 15,
		print_ucharh,
		fill_uchar, flush_uchar,
		" Number of old, SASI-compatible hard disks connected.\n"
		" Note they are not supported on NetBSD.\n"
	},
	{
		"hw", "memory",
		8, 4, 0, {longword:0}, 0, {longword:0}, {longword:1024*1024},
		parse_byte, 1024*1024, 12*1024*1024,
		print_ulongh,
		fill_ulong, flush_ulong,
		" Memory size in byte.\n"
		" Can be specified by Kilobyte and Megabyte with suffix KB and MB respectively.\n"
	},
#ifdef notyet
	{
		"hw", "serial",
		26, 2, 0, {word:{[0] 0}}, 0, {word:{[0] 0}}, {word:{[0] 0x4e07}},
		parse_serial, 0, 0,
		print_serial,
		fill_ushort, flush_ushort,
		" Serial mode."
	},
#endif
	{
		"hw", "srammode",
		45, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 1,
		print_uchar,
		fill_uchar, flush_uchar,
		" Usage of the user area of non-volatile static RAM.\n"
		" 0 for unused, 1 for SRAMDISK, 2 for user program.\n"
	},
	{
		"hw", "upcount",
		84, 4, 0, {longword:0}, 0, {longword:0}, {longword:0},
		parse_dummy, 0, 0xffffffff,
		print_ulong,
		fill_ulong, flush_dummy,
		" Boot count since the SRAM is initialized.\n"
	},
	{
		"hw", "uptime",
		76, 4, 0, {longword:0}, 0, {longword:0}, {longword:0},
		parse_dummy, 0, 0xffffffff,
		print_ulong,
		fill_ulong, flush_dummy,
		" Total uptime since the SRAM is initialized.\n"
	},
	{
		"keyboard", "delay",
		58, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 3}},
		parse_uchar, 0, 0xff,
		print_uchar,
		fill_uchar, flush_uchar,
		" Delay for start keyboard autorepeat. (200+100*n) ms.\n"
	},
	{
		"keyboard", "kanalayout",
		43,  1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 1,
		print_uchar,
		fill_uchar, flush_uchar,
		" Layout of kana keys.  0 for JIS, 1 for AIUEO order.\n"
		" Note on NetBSD kana input is not supported.\n"
	},
	{
		"keyboard", "ledstat",
		28, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 0x7f,
		print_uchar,
		fill_uchar, flush_uchar,
		" Initial keyboard LED status (bitmap).\n"
		" Each bit means KANA, ROMAJI, CODE, CAPS, INS, HIRAGANA, ZENKAKU from LSB.\n"
	},
	{
		"keyboard", "opt2",
		39, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 1,
		print_uchar,
		fill_uchar, flush_uchar,
		" 1 for normal, 0 for TV control.\n"
	},
	{
		"keyboard", "repeat",
		59, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 3}},
		parse_uchar, 0, 0xff,
		print_uchar,
		fill_uchar, flush_uchar,
		" Time elapsed between the keyboard autorepeat. (30+5*n^2 ms.\n"
	},
	{
		"poweroff", "ejectfloppy",
		41, 1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 0}},
		parse_uchar, 0, 1,
		print_uchar,
		fill_uchar, flush_uchar,
		" 1 to eject floppy disks at shutdown.\n"
	},
	{
		"poweroff", "tvctrl",
		42,  1, 0, {byte:{[0] 0}}, 0, {byte:{[0] 0}}, {byte:{[0] 13}},
		parse_uchar, 0, 0xff,
		print_uchar,
		fill_uchar, flush_uchar,
		" What to do at shutdown for display TV.\n"
	},
	{
		"printer", "timeout",
		60, 4, 0, {longword:0}, 0, {longword:0}, {longword:0x80000},
		parse_ulong, 0, 0xffffffff,
		print_ulong,
		fill_ulong, flush_ulong,
		" Printer timeout in second.\n"
	},
};

int number_of_props = sizeof (properties) / sizeof (struct property);
