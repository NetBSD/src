# $Id: Makefile.inc,v 1.3 1994/09/19 23:51:29 deraadt Exp $

SRCS+=		netbsdm68k.c netbsdm68k4k.c cpu-m68k.c trad-core.c

CFLAGS+=	-DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_m68k4k_vec \
		-DSELECT_ARCHITECTURES=bfd_m68k_arch \
		-DSELECT_VECS='&netbsd_m68k_vec, &netbsd_m68k4k_vec'


