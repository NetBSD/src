/*	$NetBSD: platform.h,v 1.3 2003/01/31 22:07:53 tsutsui Exp $	*/
/*	NetBSD: cpuconf.h,v 1.12 2000/06/08 03:10:06 thorpej Exp 	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

struct platform {
	/*
	 * Platform Information.
	 */
	const char	*system_id;	/* from root of configuration tree */
	const char	*vendor_id;	/* vendor id from GetSystemId() */
	const char	*variant;	/* Variant Name */
	const char	*model;		/* Model Name */
	const char	*vendor;	/* Vendor Name */
	int		clock;		/* CPU clock [MHz] */

	char	**mainbusdevs;

	/*
	 * Platform Specific Function Hooks
	 */
	int	(*match) __P((struct platform *));
	void	(*init) __P((void));
	void	(*cons_init) __P((void));
	void	(*reset) __P((void));
	void	(*set_intr) __P((int,
		    int (*) __P((u_int, struct clockframe *)), int));
};

int ident_platform __P((void));
int platform_generic_match __P((struct platform *));
void platform_nop __P((void));

extern struct platform *platform;

extern struct platform *const plattab[];
extern const int nplattab;

/*
 * supported platforms, sorted by alphabetic order of p_*.c filename.
 */
extern struct platform platform_acer_pica_61;
extern struct platform platform_desktech_arcstation_i;
extern struct platform platform_desktech_tyne;
extern struct platform platform_microsoft_jazz;
extern struct platform platform_nec_j96a;
extern struct platform platform_nec_jc94;
extern struct platform platform_nec_r94;
extern struct platform platform_nec_r96;
extern struct platform platform_nec_riscserver_2200;
extern struct platform platform_nec_rax94;
extern struct platform platform_nec_rd94;
extern struct platform platform_sni_rm200pci;

void c_isa_init __P((void));
void c_isa_cons_init __P((void));

extern char *c_jazz_eisa_mainbusdevs[];
void c_jazz_eisa_init __P((void));
void c_jazz_eisa_cons_init __P((void));

void c_magnum_set_intr __P((int, int (*) __P((u_int, struct clockframe *)),
    int));
void c_magnum_init __P((void));

void c_nec_eisa_init __P((void));
void c_nec_eisa_cons_init __P((void));

void c_nec_jazz_set_intr __P((int, int (*) __P((u_int, struct clockframe *)),
    int));
void c_nec_jazz_init __P((void));

extern char *c_nec_pci_mainbusdevs[];
void c_nec_pci_init __P((void));
void c_nec_pci_cons_init __P((void));


