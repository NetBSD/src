/* $NetBSD: autoconf.h,v 1.3.126.1 2009/12/08 01:58:43 cyber Exp $ */

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
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
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
 * Machine-dependent structures for autoconfiguration
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

#if _LP64
#define BI_FIXUP(x) ((vaddr_t)x | 0xffffffff00000000)
#else
#define BI_FIXUP(x) x
#endif
 

struct bootinfo_v1 {
	uint32_t	version;	/*  0: version of bootinfo	 */
	uint32_t	reserved;	/*  4: round to 64-bit boundary	 */
	uint32_t	ssym;		/*  8: start of kernel sym table */
	uint32_t	esym;		/* 12: end of kernel sym table	 */
	char	boot_flags[64];		/* 16: boot flags		 */
	char	booted_kernel[64];	/* 80: name of booted kernel	 */
	uint32_t	fwhandle;	/* 144: firmware handle		 */
	uint32_t	fwentry;	/* 148: firmware entry point	 */
	u_char	reserved2[100];		/* 256: total size		 */
};


struct bootinfo_v1_int {
	uint32_t	version;	/*   0/0: version of bootinfo	    */
	uint32_t	reserved;	/*   4/4: round to 64-bit boundary  */
	vaddr_t	ssym;			/*   8/8: start of kernel sym table */
	vaddr_t	esym;			/* 12/16: end of kernel sym table   */
	char	boot_flags[64];		/* 16/24: boot flags		    */
	char	booted_kernel[64];	/* 80/88: name of booted kernel	    */
	vaddr_t	fwhandle;		/* 144/152: firmware handle	    */
	vaddr_t	fwentry;		/* 148/160: firmware entry point    */
#ifdef _LP64
	u_char	reserved2[88];		/* 168: total size -> 256	    */
#else
	u_char	reserved2[104];		/* 152: total size -> 256	    */
#endif
};

