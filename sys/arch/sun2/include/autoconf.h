/*	$NetBSD: autoconf.h,v 1.3 2001/06/27 19:21:49 fredette Exp $	*/

#include <sun68k/autoconf.h>

#define	mbio_attach_args	mainbus_attach_args
#define	mba_bustag		ma_bustag
#define	mba_dmatag		ma_dmatag
#define	mba_name		ma_name
#define	mba_paddr		ma_paddr
#define	mba_pri			ma_pri

#define	mbmem_attach_args	mainbus_attach_args
#define	mbma_bustag		ma_bustag
#define	mbma_dmatag		ma_dmatag
#define	mbma_name		ma_name 
#define	mbma_paddr		ma_paddr
#define	mbma_pri		ma_pri
