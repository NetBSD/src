/*	$NetBSD: bootxx.h,v 1.1.1.1 1996/02/29 11:35:06 leo Exp $	*/

/*
 * Copyright (c) 1995 L. Weppelman
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TOS variables in low memory
 */
struct _gem_mupb;
struct _basepage;
struct _osheader {
	u_int16_t	 os_entry;
	u_int16_t	 os_version;
	void		 *reseth;
	struct _osheader *os_beg;
	void		 *os_end;
	u_int32_t	 os_rsv1;
	struct _gem_mupb *os_magic;
	u_int32_t	 os_date;
	u_int16_t	 os_conf;
	u_int16_t	 os_dosdate;
	char		 **p_root;
	u_int8_t	 **pkbshift;
	struct _basepage **p_run;
	char		 *p_rsv2;
};

#define ADDR_SYSBASE	((struct _osheader**)0x4f2) /* TOS header	*/
#define	ADDR_P_COOKIE	((long**)0x5a0)	/* Pointer to cookie jar	*/
#define	ADDR_PHYSTOP	((long*)0x42e)	/* End of ST-ram		*/
#define	ADDR_RAMTOP	((long*)0x5a4)	/* End of TT-ram (unofficial)	*/
#define	ADDR_CHKRAMTOP	((long*)0x5a8)	/*  above is valid (unofficial)	*/

#define	RAMTOP_MAGIC	(0x1357bd13)	/* Magic no. for ADDR_CHKRAMTOP	*/
#define	TTRAM_BASE	(0x1000000)	/* Fastram always starts here	*/

/*
 * Kernel parameter block
 */
typedef struct kparamb	kparb;
typedef struct {
	const char *	ostype;
	const char *	osname;
	unsigned	rootfs;
	kparb 		kp;
} osdsc;

#define	ksize		kp.ksize
#define	kstart		kp.kp
#define	kentry		kp.entry
#define	k_esym		kp.esym_loc
#define	stmem_size	kp.stmem_size
#define	ttmem_size	kp.ttmem_size
#define	ttmem_start	kp.ttmem_start
#define	cputype		kp.bootflags
#define	boothowto	kp.boothowto
