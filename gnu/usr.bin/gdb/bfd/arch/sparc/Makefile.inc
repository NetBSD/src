# $Id: Makefile.inc,v 1.3 1994/09/19 23:51:32 deraadt Exp $

SRCS+=		netbsd_sparc.c cpu-sparc.c

CFLAGS+=	-DNETBSD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_sparc_vec \
		-DSELECT_ARCHITECTURES=bfd_sparc_arch \
		-DSELECT_VECS='&netbsd_sparc_vec'


