# $Id: Makefile.inc,v 1.1 1994/01/28 12:38:23 pk Exp $

SRCS+=		netbsdm68k.c cpu-m68k.c

CFLAGS+=	-DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_m68k_vec \
		-DSELECT_ARCHITECTURES=bfd_m68k_arch \
		-DSELECT_VECS='&netbsd_m68k_vec'


