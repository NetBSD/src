/*
 *	definitions for exec_kernel()
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: execkern.h,v 1.1 1998/09/01 19:51:56 itohy Exp $
 */

#ifndef X68K_BOOT_EXECKERN_H
#define X68K_BOOT_EXECKERN_H

struct execkern_arg {
	void		*image_top;
	u_long		load_addr;
	u_long		text_size;
	u_long		data_size;
	u_long		bss_size;
	u_long		symbol_size;
	unsigned	d5;		/* reserved */
	int		rootdev;
	u_long		boothowto;
	u_long		entry_addr;
};

void __dead exec_kernel __P((struct execkern_arg *)) __attribute__((noreturn));

#endif /* X68K_BOOT_EXECKERN_H */
