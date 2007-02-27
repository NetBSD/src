/*	$NetBSD: device_test.c,v 1.2.8.1 2007/02/27 16:50:34 yamt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/sbd.h>
#include <machine/gavar.h>

#include "console.h"	/* zskbd_print_keyscan */
#include "cmd.h"
#include "local.h"
#include "common.h"

struct ga ga;

extern bool lance_test(void);
int fdd_test(void);
int i82589_test(void);
int picnic_test(void);
int beep_test(void);
int clut_test(int);

int
cmd_ga_test(int argc, char *argp[], int interactive)
{
	int i;

	switch (strtoul(argp[1], 0, 0)) {
	default:
		break;
	case 0:
		if (SBD_INFO->machine == MACHINE_TR2A)
			ga.flags = 0x0001;
		ga_init(&ga);
		break;
	case 1:
		for (i = 0; i < 256; i++)
			fb_clear(240, 240, 240, 240, i);
		break;
	case 2:
		ga_plane_mask_test(&ga);
		break;
	}

	return 0;
}

int
cmd_kbd_scancode(int argc, char *argp[], int interactive)
{

	if (argc < 2) {
		printf("ks on/off\n");
		return 1;
	}

	if (argp[1][1] == 'n')
		zskbd_print_keyscan(1);
	else if (argp[1][1] == 'f')
		zskbd_print_keyscan(0);

	return 0;
}

int
cmd_ether_test(int argc, char *argp[], int interactive)
{

	if (SBD_INFO->machine == MACHINE_TR2)
		i82589_test();
	if (SBD_INFO->machine == MACHINE_TR2A)
		lance_test();

	return 0;
}

int
i82589_test(void)
{
#define	SCP_ADDR	0xa0800000
#define	IEE_ADDR	((volatile uint32_t *)0xbb060000)
	volatile uint32_t cmd;
	volatile uint16_t u16;
	int i;

	/* Board reset */
	*IEE_ADDR = 0;
	*IEE_ADDR = 0;

	for (i = 0; i < 100; i++)
		;

	/* Board self test */
	*(uint32_t *)(SCP_ADDR + 4) = 0xffffffff;
	cmd = (SCP_ADDR & 0x0ffffff0) | /* i82586-mode, A24-A31 are omitted */
	    0x1/*Self test */;
	cmd = (cmd << 16) | (cmd >> 16);
	printf("self test command 0x%x\n", cmd);
	*IEE_ADDR = cmd;
	*IEE_ADDR = cmd;
	for (i = 0; i < 0x60000; i++) 	/* 393216 */
		if ((u16 = *(volatile uint16_t *)(SCP_ADDR + 6)) == 0)
			break;

	if (i != 0x60000) {
		printf("i82596 self test success. (loop %d)\n", i);
	} else {
		printf("i82586 self test failed.\n");
		return 1;
	}

	u16 = *(uint16_t *)(SCP_ADDR + 4);
	printf("SCP_ADDR+4=0x%x\n", u16);
	if ((u16 & 0x103c) != 0) {
		printf("i82589 self test data error.\n");
		return 1;
	} else {
		printf("i82589 self test data OK.\n");
	}

	return 0;
}

int
picnic_test(void)
{

	/* PICNIC test */
	printf("--------------------\n");
	printf("%x ", *(uint8_t *)0xbb000000); /* 0x00 */
	printf("%x ", *(uint8_t *)0xbb000004); /* 0xc0 */
	printf("%x ", *(uint8_t *)0xbb000008); /* 0x00 */
	printf("%x ", *(uint8_t *)0xbb000010); /* 0x01 */
	printf("\n");
	printf("%x ", *(uint8_t *)0xbb001000); /* 0x00 */
	printf("%x ", *(uint8_t *)0xbb001004); /* 0x00 */
	printf("%x ", *(uint8_t *)0xbb001008); /* 0x00 */
	printf("%x\n", *(uint8_t *)0xbb001010);/* 0x01 */
	printf("--------------------\n");
	*(uint8_t *)0xbb001010 = 0;
#if 0
	*(uint32_t *)0xbb060000 = 0;
#endif
	printf("--------------------\n");
	printf("%x ", *(uint8_t *)0xbb000000);
	printf("%x ", *(uint8_t *)0xbb000004);
	printf("%x ", *(uint8_t *)0xbb000008);
	printf("%x ", *(uint8_t *)0xbb000010);
	printf("\n");
	printf("%x ", *(uint8_t *)0xbb001000);
	printf("%x ", *(uint8_t *)0xbb001004);
	printf("%x ", *(uint8_t *)0xbb001008);
	printf("%x\n", *(uint8_t *)0xbb001010);
	printf("--------------------\n");

	return 0;
}

int
beep_test(void)
{

	/* Beep test */
	for (;;)
		*(volatile uint8_t *)0xbb007000 = 0xff;

	return 0;
}

int
fdd_test(void)
{

	/* ROM_FDRDWR test */
	uint8_t buf[512];
	int i, err, cnt;
	uint32_t pos;

	for (i = 0; i < 1989; i++) {
		memset(buf, 0, 512);
		blk_to_2d_position(i, &pos, &cnt);
		err = ROM_FD_RW(0, pos, cnt, buf);
		printf("%d err=%d\n", i, err);
		if (err != 0)
			break;
#if 0
		for (j = 0; j < 512; j++)
			printf("%x", buf[j]);
		printf("\n");
#else
		printf("%s\n", buf);
#endif
	}
	return 0;
}
