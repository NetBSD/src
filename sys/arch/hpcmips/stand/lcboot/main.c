/* $NetBSD: main.c,v 1.6.14.1 2017/12/03 11:36:15 jdolecek Exp $ */

/*
 * Copyright (c) 2003 Naoto Shimazaki.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY NAOTO SHIMAZAKI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE NAOTO OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Boot loader for L-Card+
 *
 * ROM Map
 * -------
 * ROM1
 * BFFF FFFF	------------------------------
 *
 *		reserved
 *
 * BF80 0000	------------------------------
 *
 * ROM0
 * BFFF FFFF	------------------------------
 *
 *		user storage (max 2Mbytes)
 *
 * BFE0 0000	------------------------------
 *
 *		reserved
 *
 * BFD4 0000	------------------------------
 *
 *		boot params
 *
 * BFD2 0000	------------------------------
 *
 *		second boot loader (mirror image)
 *		or Linux Kernel
 *
 * BFD0 0000	------------------------------
 *
 *		first boot loader (L-Card+ original loader)
 *
 *		reset vector
 * BFC0 0000	------------------------------
 *
 *		gziped kernel image (max 4Mbytes)
 *
 * BF80 0000	------------------------------
 *
 *
 *
 * RAM Map
 * -------
 *
 * 80FF FFFF	------------------------------
 *		ROM ICE work
 * 80FF FE00	------------------------------
 *		ROM ICE stack
 * 80FF FDA8	------------------------------
 *
 *
 *
 *		kernel
 * 8004 0000	------------------------------
 *		kernel stack (growing to lower)
 *
 *
 *		boot loader heap (growing to upper)
 *		boot loader text & data (at exec time)
 * 8000 1000	------------------------------
 *		vector table
 * 8000 0000	------------------------------
 *
 *		virtual memory space
 *
 * 0000 0000	------------------------------
 *
 *
 *
 * ROMCS0 <-> ROMCS3 mapping
 *
 *  ROMCS0        ROMCS3
 * BE7F FFFF <-> BFFF FFFF
 * BE40 0000 <-> BFC0 0000	reset vector
 * BE00 0000 <-> BF80 0000
 * 
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: main.c,v 1.6.14.1 2017/12/03 11:36:15 jdolecek Exp $");

#include <lib/libsa/stand.h>

#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/cmureg.h>
#include <hpcmips/vr/vr4181giureg.h>

#include "extern.h"
#include "i28f128reg.h"

/* XXX */
#define ISABRGCTL	0x00
#define ISABRGSTS	0x02
#define XISACTL		0x04

#define BOOTTIMEOUT	9	/* must less than 10 */
#define LINEBUFLEN	80

extern const char bootprog_rev[];
extern const char bootprog_name[];

static void command_help(char *opt);
static void command_dump(char *opt);
static void command_boot(char *opt);
static void command_load(char *opt);
static void command_fill(char *opt);
static void command_write(char *opt);
static void command_option(char *subcmd);
static void opt_subcmd_print(char *opt);
static void opt_subcmd_read(char *opt);
static void opt_subcmd_write(char *opt);
static void opt_subcmd_path(char *opt);
static void opt_subcmd_bootp(char *opt);
static void opt_subcmd_ip(char *opt);


struct boot_option	bootopts;

static struct bootmenu_command commands[] = {
	{ "?",		command_help },
	{ "h",		command_help },
	{ "d",		command_dump },
	{ "b",		command_boot },
	{ "l",		command_load },
	{ "f",		command_fill },
	{ "w",		command_write },
	{ "o",		command_option },
	{ NULL,		NULL },
};

static struct bootmenu_command opt_subcommands[] = {
	{ "p",		opt_subcmd_print },
	{ "r",		opt_subcmd_read },
	{ "w",		opt_subcmd_write },
	{ "path",	opt_subcmd_path },
	{ "bootp",	opt_subcmd_bootp },
	{ "ip",		opt_subcmd_ip },
	{ NULL,		NULL },
};

static void
print_banner(void)
{
	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
#if 0
	printf(">> Memory: %d/%d k\n", getbasemem(), getextmem());
#endif
}

static void
init_devices(void)
{
	/* Init RTC */
	REGWRITE_2(VRETIMEH, 0, 0);
	REGWRITE_2(VRETIMEM, 0, 0);
	REGWRITE_2(VRETIMEL, 0, 0);


	/*
	 * CLKSPEEDREG	0x6012
	 *	DIV	DIV2 mode
	 *	CLKSP	18 (0x12)
	 *	PClock (CPU clock)		65.536MHz
	 *		PClock = (18.432MHz / CLKSP) x 64
	 *		       = (18.432MHz / 18) x 64
	 *		       = 65.536MHz
	 *	TClock (peripheral clock)	32.768MHz
	 *		TClock = PClock / DIV
	 *		       = 65.536MHz / 2
	 *		       = 32.768MHz
	 */

	/*
	 * setup ISA BUS clock freqency
	 *
	 * set PCLK (internal peripheral clock) to 32.768MHz (TClock / 1)
	 * set External ISA bus clock to 10.922MHz (TClock / 3)
	 */
	REGWRITE_2(VR4181_ISABRG_ADDR, ISABRGCTL, 0x0003);
	REGWRITE_2(VR4181_ISABRG_ADDR, XISACTL, 0x0401);

	/*
	 * setup peripheral's clock supply
	 *
	 * CSU: disable
	 * AIU: enable (AIU, ADU, ADU18M)
	 * PIU: disable
	 * SIU: enable (SIU18M)
	 */
	REGWRITE_2(VR4181_CMU_ADDR, 0, CMUMASK_SIU | CMUMASK_AIU);

	/*
	 * setup GPIO
	 */
#if 0
	/* L-Card+ generic setup */
	/*
	 * pin   mode	comment
	 * GP0 : GPI	not used
	 * GP1 : GPI	not used
	 * GP2 : GPO	LED6 (0: on  1: off)
	 * GP3 : PCS0	chip select for CS8900A Lan controller
	 * GP4 : GPI	IRQ input from CS8900A
	 * GP5 : GPI	not used
	 * GP6 : GPI	not used
	 * GP7 : GPI	reserved by TANBAC TB0193
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_PIOD_L_REG_W, 0xffff);
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE0_REG_W,
		   GP3_PCS0 | GP2_GPO);
	/*
	 * pin   mode	comment
	 * GP8 : GPO	LED5 (0: on  1: off)
	 * GP9 : GPI	CD2
	 * GP10: GPI	CD1
	 * GP11: GPI	not used
	 * GP12: GPI	not used
	 * GP13: GPI	not used
	 * GP14: GPI	not used
	 * GP15: GPI	not used
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE1_REG_W, GP8_GPO);
	/*
	 * pin   mode	comment
	 * GP16: IORD	ISA bus
	 * GP17: IOWR	ISA bus
	 * GP18: IORDY	ISA bus
	 * GP19: GPI	not used
	 * GP20: GPI	not used
	 * GP21: RESET	resets CS8900A
	 * GP22: ROMCS0	ROM chip select
	 * GP23: ROMCS1	ROM chip select
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE2_REG_W,
		   GP23_ROMCS1 | GP22_ROMCS0 | GP21_RESET
		   | GP18_IORDY | GP17_IOWR | GP16_IORD);
	/*
	 * GP24: ROMCS2	ROM chip select
	 * GP25: RxD1	SIU1
	 * GP26: TxD1	SIU1
	 * GP27: RTS1	SIU1
	 * GP28: CTS1	SIU1
	 * GP29: GPI	LED3
	 * GP30: GPI	reserved by TANBAC TB0193
	 * GP31: GPI	LED4
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE3_REG_W,
		   GP30_GPI
		   | GP28_CTS1 | GP27_RTS1 | GP26_TxD1 | GP25_RxD1
		   | GP24_ROMCS2);
#else
	/* e-care node specific setup */
	/*
	 * pin   mode	comment
	 * GP0 : GPO	ECNRTC_RST
	 * GP1 : GPO	ECNRTC_CLK
	 * GP2 : GPO	LED6 (0: on  1: off)
	 * GP3 : PCS0	chip select for CS8900A Lan controller
	 * GP4 : GPI	IRQ input from CS8900A
	 * GP5 : GPO	ECNRTC_DIR
	 * GP6 : GPO	ECNRTC_OUT
	 * GP7 : GPI	reserved by TANBAC TB0193
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_PIOD_L_REG_W, 0xffff);
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE0_REG_W,
		   GP6_GPO | GP5_GPO | GP3_PCS0
		   | GP2_GPO | GP1_GPO | GP0_GPO);

	/*
	 * pin   mode	comment
	 * GP8 : GPO	LED5 (0: on  1: off)
	 * GP9 : GPI	CD2
	 * GP10: GPI	CD1
	 * GP11: GPI	not used
	 * GP12: GPI	ECNRTC_IN
	 * GP13: GPI	not used
	 * GP14: GPI	not used
	 * GP15: GPI	not used
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE1_REG_W,
		   GP12_GPI | GP8_GPO);

	/*
	 * pin   mode	comment
	 * GP16: IORD	ISA bus
	 * GP17: IOWR	ISA bus
	 * GP18: IORDY	ISA bus
	 * GP19: GPI	not used
	 * GP20: GPI	not used
	 * GP21: RESET	resets CS8900A
	 * GP22: ROMCS0	ROM chip select
	 * GP23: ROMCS1	ROM chip select
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE2_REG_W,
		   GP23_ROMCS1 | GP22_ROMCS0 | GP21_RESET
		   | GP18_IORDY | GP17_IOWR | GP16_IORD);
	/*
	 * GP24: ROMCS2	ROM chip select
	 * GP25: RxD1	SIU1
	 * GP26: TxD1	SIU1
	 * GP27: RTS1	SIU1
	 * GP28: CTS1	SIU1
	 * GP29: GPI	LED3
	 * GP30: GPI	reserved by TANBAC TB0193
	 * GP31: GPI	LED4
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_MODE3_REG_W,
		   GP30_GPI
		   | GP28_CTS1 | GP27_RTS1 | GP26_TxD1 | GP25_RxD1
		   | GP24_ROMCS2);
#endif

#if 0
	/*
	 * setup interrupt
	 *
	 * I4TYP:  falling edge trigger
	 * GIMSK4: unmask
	 * GIEN4:  enable
	 * other:  unused, mask, disable
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_INTTYP_L_REG_W,
		   I4TYP_HIGH_LEVEL);
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_INTMASK_REG_W,
		   0xffffU & ~GIMSK4);
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_INTEN_REG_W, GIEN4);
#endif

	/*
	 * programmable chip select
	 *
	 * PCS0 is used to select CS8900A Ethernet controller
	 * on TB0193
	 *
	 * PCS0:
	 *	0x14010000 - 0x14010fff
	 *	I/O access, 16bit cycle, both of read/write
	 * PCS1: unused
	 */
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_PCS0STRA_REG_W, 0x0000);
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_PCS0STPA_REG_W, 0x0fff);
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_PCS0HIA_REG_W, 0x1401);
	REGWRITE_2(VR4181_GIU81_ADDR, VR4181GIU_PCSMODE_REG_W,
		   PCS0MIOB_IO | PCS0DSIZE_16BIT | PCS0MD_READWRITE);
}

/*
 * chops the head from the arguments and returns the arguments if any,
 * or possibly an empty string.
 */
static char *
get_next_arg(char *arg)
{
	char *opt;

	if ((opt = strchr(arg, ' ')) == NULL) {
		opt = "";
	} else {
		*opt++ = '\0';
	}

        /* trim leading blanks */
	while (*opt == ' ')
		opt++;

	return opt;
}

static void
command_help(char *opt)
{
	printf("commands are:\n"
	       "boot:\tb\n"
	       "dump:\td addr [addr]\n"
	       "fill:\tf addr addr char\n"
	       "load:\tl [offset] (with following S-Record)\n"
	       "write:\tw dst src len\n"
	       "option:\to subcommand [params]\n"
	       "help:\th|?\n"
	       "\n"
	       "option subcommands are:\n"
	       "print:\to p\n"
	       "read:\to r\n"
	       "write:\to w\n"
	       "path:\to path pathname\n"
	       "bootp:\to bootp yes|no\n"
	       "ip:\to ip remote local netmask gateway\n"
		);
}

static void
bad_param(void)
{
	printf("bad param\n");
	command_help(NULL);
}

static const u_int8_t print_cnv[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static void
printhexul(u_int32_t n)
{
	int	i;

	for (i = 28; i >= 0; i -= 4)
		putchar(print_cnv[(n >> i) & 0x0f]);
}

static void
printhexuc(u_int8_t n)
{
	int	i;

	for (i = 4; i >= 0; i -= 4)
		putchar(print_cnv[(n >> i) & 0x0f]);
}

static void
command_dump(char *opt)
{
	char		*endptr;
	const char	*p;
	const char	*line_fence;
	const char	*limit;

	p = (const char *) strtoul(opt, &endptr, 16);
	if (opt == endptr) {
		bad_param();
		return;
	}

	opt = get_next_arg(opt);
	limit = (const char *) strtoul(opt, &endptr, 16);
	if (opt == endptr) {
		limit = p + 256;
	}

	for (;;) {
		printhexul((u_int32_t) p);
		putchar(' ');
		line_fence = p + 16;
		while (p < line_fence) {
			printhexuc(*p++);
			putchar(' ');
			if (p >= limit) {
				putchar('\n');
				return;
			}
		}
		putchar('\n');
		if (ISKEY) {
			if (getchar() == '\x03')
				break;
		}
	}
}

static void
command_boot(char *opt)
{
	u_long	marks[MARK_MAX];

	marks[MARK_START] = 0;
	if (loadfile(bootopts.b_pathname, marks, LOAD_KERNEL)) {
		printf("loadfile failed\n");
		return;
	}
	start_netbsd();
	/* no return */
}

/*
 * loading S-Record
 */
static int
load_srec(char *offset)
{
	char		s2lbuf[9];
	char		c;
	char		rectype;
	u_int32_t	reclen;
	u_int32_t	reclen_bk;
	u_int32_t	recaddr;
	char		*endptr;
	char		*p;
	u_int32_t	sum;
	int		err = 0;

	for (;;) {
		/*
		 * the first step is to read a S-Record.
		 */
		if ((c = getchar()) != 'S')
			goto out;

		rectype = getchar();

		s2lbuf[0] = getchar();
		s2lbuf[1] = getchar();
		s2lbuf[2] = '\0';
		reclen_bk = reclen = strtoul(s2lbuf, &endptr, 16);
		if (endptr != &s2lbuf[2])
			goto out;
		sum = reclen;

		p = s2lbuf;

		switch (rectype) {
		case '0':
			/* just ignore */
			do {
				c = getchar();
			} while (c != '\r' && c != '\n');
			continue;

		case '3':
			*p++ = getchar();
			*p++ = getchar();
			reclen--;
			/* FALLTHRU */
		case '2':
			*p++ = getchar();
			*p++ = getchar();
			reclen--;
			/* FALLTHRU */
		case '1':
			*p++ = getchar();
			*p++ = getchar();
			*p++ = getchar();
			*p++ = getchar();
			*p = '\0';
			reclen -= 2;

			recaddr = strtoul(s2lbuf, &endptr, 16);
			if (endptr != p)
				goto out;
			sum += (recaddr >> 24) & 0xff;
			sum += (recaddr >> 16) & 0xff;
			sum += (recaddr >> 8) & 0xff;
			sum += recaddr & 0xff;

			p = offset + recaddr;
			/*
			 * XXX
			 * address range is must be chaked here!
			 */
			reclen--;
			s2lbuf[2] = '\0';
			while (reclen > 0) {
				s2lbuf[0] = getchar();
				s2lbuf[1] = getchar();
				*p = (u_int8_t) strtoul(s2lbuf, &endptr, 16);
				if (endptr != &s2lbuf[2])
					goto out;
				sum += *p++;
				reclen--;
			}
			break;

		case '7':
		case '8':
		case '9':
			goto out2;

		default:
			goto out;
		}

		s2lbuf[0] = getchar();
		s2lbuf[1] = getchar();
		s2lbuf[2] = '\0';
		sum += (strtoul(s2lbuf, &endptr, 16) & 0xff);
		sum &= 0xff;
		if (sum != 0xff) {
			printf("checksum error\n");
			err = 1;
			goto out2;
		}

		c = getchar();
		if (c != '\r' && c != '\n')
			goto out;
	}
	/* never reach */
	return 1;

out:
	printf("invalid S-Record\n");
	err = 1;

out2:
	do {
		c = getchar();
	} while (c != '\r' && c != '\n');

	return err;
}

static void
command_load(char *opt)
{
	char	*endptr;
	char	*offset;
	
	offset = (char *) strtoul(opt, &endptr, 16);
	if (opt == endptr)
		offset = 0;
	load_srec(offset);
}

static void
command_fill(char *opt)
{
	char	*endptr;
	char	*p;
	char	*limit;
	int	c;

	p = (char *) strtoul(opt, &endptr, 16);
	if (opt == endptr) {
		bad_param();
		return;
	}

	opt = get_next_arg(opt);
	limit = (char *) strtoul(opt, &endptr, 16);
	if (opt == endptr) {
		bad_param();
		return;
	}

	opt = get_next_arg(opt);
	c = strtoul(opt, &endptr, 16);
	if (opt == endptr)
		c = '\0';

	memset(p, c, limit - p);
}

static void
check_write_verify_flash(u_int32_t src, u_int32_t dst, size_t len)
{
	int		status;

	if ((dst & I28F128_BLOCK_MASK) != 0) {
		printf("dst addr must be aligned to block boundary (0x%x)\n",
		       I28F128_BLOCK_SIZE);
		return;
	}

	if (i28f128_probe((void *) dst)) {
		printf("dst addr is not a intel 28F128\n");
	} else {
		printf("intel 28F128 detected\n");
	}

	if ((status = i28f128_region_write((void *) dst, (void *) src, len))
	    != 0) {
		printf("write mem to flash failed status = %x\n", status);
		return;
	}

	printf("verifying...");
	if (memcmp((void *) dst, (void *) src, len)) {
		printf("verify error\n");
		return;
	}
	printf("ok\n");

	printf("writing memory to flash succeeded\n");
}

static void
command_write(char *opt)
{
	char		*endptr;
	u_int32_t	src;
	u_int32_t	dst;
	size_t		len;

	dst = strtoul(opt, &endptr, 16);
	if (opt == endptr)
		goto out;

	opt = get_next_arg(opt);
	src = strtoul(opt, &endptr, 16);
	if (opt == endptr)
		goto out;

	opt = get_next_arg(opt);
	len = strtoul(opt, &endptr, 16);
	if (opt == endptr)
		goto out;

	check_write_verify_flash(src, dst, len);
	return;

out:
	bad_param();
	return;
}

static void
command_option(char *subcmd)
{
	char	*opt;
	int	i;

	opt = get_next_arg(subcmd);

	/* dispatch subcommand */
	for (i = 0; opt_subcommands[i].c_name != NULL; i++) {
		if (strcmp(subcmd, opt_subcommands[i].c_name) == 0) {
			opt_subcommands[i].c_fn(opt);
			break;
		}
	}
	if (opt_subcommands[i].c_name == NULL) {
		printf("unknown option subcommand\n");
		command_help(NULL);
	}
}

static void
opt_subcmd_print(char *opt)
{
	printf("boot options:\n"
	       "magic:\t\t%s\n"
	       "pathname:\t`%s'\n"
	       "bootp:\t\t%s\n",
	       bootopts.b_magic == BOOTOPT_MAGIC ? "ok" : "bad",
	       bootopts.b_pathname,
	       bootopts.b_flags & B_F_USE_BOOTP ? "yes" : "no");
	printf("remote IP:\t%s\n", inet_ntoa(bootopts.b_remote_ip));
	printf("local IP:\t%s\n", inet_ntoa(bootopts.b_local_ip));
	printf("netmask:\t%s\n", intoa(bootopts.b_netmask));
	printf("gateway IP:\t%s\n", inet_ntoa(bootopts.b_gate_ip));
}

static void
opt_subcmd_read(char *opt)
{
	bootopts = *((struct boot_option *) BOOTOPTS_BASE);
	if (bootopts.b_magic != BOOTOPT_MAGIC)
		bootopts.b_pathname[0] = '\0';
}

static void
opt_subcmd_write(char *opt)
{
	bootopts.b_magic = BOOTOPT_MAGIC;

	check_write_verify_flash((u_int32_t) &bootopts, BOOTOPTS_BASE,
				 sizeof bootopts);
}

static void
opt_subcmd_path(char *opt)
{
	strlcpy(bootopts.b_pathname, opt, sizeof bootopts.b_pathname);
}

static void
opt_subcmd_bootp(char *opt)
{
	if (strcmp(opt, "yes") == 0) {
		bootopts.b_flags |= B_F_USE_BOOTP;
	} else if (strcmp(opt, "no") == 0) {
		bootopts.b_flags &= ~B_F_USE_BOOTP;
	} else {
		bad_param();
	}
}

static void
opt_subcmd_ip(char *opt)
{
	bootopts.b_remote_ip.s_addr = inet_addr(opt);
	opt = get_next_arg(opt);
	bootopts.b_local_ip.s_addr = inet_addr(opt);
	opt = get_next_arg(opt);
	bootopts.b_netmask = inet_addr(opt);
	opt = get_next_arg(opt);
	bootopts.b_gate_ip.s_addr = inet_addr(opt);
}

static void
bootmenu(void)
{
	char	input[LINEBUFLEN];
	char	*cmd;
	char	*opt;
	int	i;

	for (;;) {

		/* input a line */
		input[0] = '\0';
		printf("> ");
		kgets(input, sizeof(input));
		cmd = input;

		/* skip leading whitespace. */
		while(*cmd == ' ')
			cmd++;

		if(*cmd) {
			/* here, some command entered */

			opt = get_next_arg(cmd);

			/* dispatch command */
			for (i = 0; commands[i].c_name != NULL; i++) {
				if (strcmp(cmd, commands[i].c_name) == 0) {
					commands[i].c_fn(opt);
					break;
				}
			}
			if (commands[i].c_name == NULL) {
				printf("unknown command\n");
				command_help(NULL);
			}
		}
		
	}
}

static char
awaitkey(void)
{
	int	i;
	int	j;
	char	c = 0;

	while (ISKEY)
		getchar();

	for (i = BOOTTIMEOUT; i > 0; i--) {
		printf("%d\b", i);
		for (j = 0; j < 1000000; j++) {
			if (ISKEY) {
				while (ISKEY)
					c = getchar();
				goto out;
			}
		}
	}

out:
	printf("0\n");
	return(c);
}

__dead void
_rtt(void)
{
	for (;;)
		;
}

int
main(void)
{
	char	c;

	init_devices();

	comcninit();

	opt_subcmd_read(NULL);

	print_banner();

	c = awaitkey();
	if (c != '\r' && c != '\n' && c != '\0') {
		printf("type \"?\" or \"h\" for help.\n");
		bootmenu(); /* does not return */
	}

	command_boot(NULL);
	/*
	 * command_boot() returns only if it failed to boot.
	 * we enter to boot menu in this case.
	 */
	bootmenu();
	
	return 0;
}
