# $Id: Makefile.inc,v 1.2 1994/05/25 11:24:14 pk Exp $

SRCS+=		netbsd_sparc.c cpu-sparc.c

CFLAGS+=	-DNETBSD_CORE -DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_sparc_vec \
		-DSELECT_ARCHITECTURES=bfd_sparc_arch \
		-DSELECT_VECS='&netbsd_sparc_vec'


