# $Id: Makefile.inc,v 1.1 1994/04/28 17:11:00 phil Exp $

SRCS+=		netbsd-ns32k.c cpu-ns32k.c

CFLAGS+=	-DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_ns32k_vec \
		-DSELECT_ARCHITECTURES=bfd_ns32k_arch \
		-DSELECT_VECS='&netbsd_ns32k_vec'


