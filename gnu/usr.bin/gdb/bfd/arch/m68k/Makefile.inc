# $Id: Makefile.inc,v 1.2 1994/02/04 18:08:13 mycroft Exp $

SRCS+=		netbsdm68k.c netbsdm68k4k.c cpu-m68k.c

CFLAGS+=	-DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_m68k4k_vec \
		-DSELECT_ARCHITECTURES=bfd_m68k_arch \
		-DSELECT_VECS='&netbsd_m68k_vec, &netbsd_m68k4k_vec'


