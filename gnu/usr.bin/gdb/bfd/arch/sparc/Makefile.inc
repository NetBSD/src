# $Id: Makefile.inc,v 1.1 1994/01/28 12:38:28 pk Exp $

SRCS+=		netbsd_sparc.c cpu-sparc.c

CFLAGS+=	-DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_sparc_vec \
		-DSELECT_ARCHITECTURES=bfd_sparc_arch \
		-DSELECT_VECS='&netbsd_sparc_vec'


