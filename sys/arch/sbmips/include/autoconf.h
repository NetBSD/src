/* $NetBSD: autoconf.h,v 1.1.10.2 2002/06/23 17:40:06 jdolecek Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-dependent structures of autoconfiguration
 */

/*
 * The boot program passes a pointer to a bootinfo structure to the
 * kernel using the following convention:
 *
 *	a0 - CFE handle
 *	a1 - bootinfo magic
 *	a2 - pointer to bootinfo_v1 structrure
 *	a3 - reserved, 0
 */

#define	BOOTINFO_MAGIC			0x1234ABCD
#define	BOOTINFO_VERSION		1

struct bootinfo_v1 {
	u_long	version;		/*  0: version of bootinfo	 */
	u_long	reserved;		/*  4: round to 64-bit boundary	 */
	u_long	ssym;			/*  8: start of kernel sym table */
	u_long	esym;			/* 12: end of kernel sym table	 */
	char	boot_flags[64];		/* 16: boot flags		 */
	char	booted_kernel[64];	/* 80: name of booted kernel	 */
	u_long	fwhandle;		/* 144: firmware handle		 */
	u_long	fwentry;		/* 148: firmware entry point	 */
	u_char	reserved2[100];		/* 256: total size		 */
};
