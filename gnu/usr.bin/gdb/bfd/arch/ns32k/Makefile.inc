# $Id: Makefile.inc,v 1.2 1994/09/19 23:51:31 deraadt Exp $

SRCS+=		netbsd-ns32k.c cpu-ns32k.c trad-core.c

CFLAGS+=	-DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_ns32k_vec \
		-DSELECT_ARCHITECTURES=bfd_ns32k_arch \
		-DSELECT_VECS='&netbsd_ns32k_vec'


