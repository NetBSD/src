# $Id: Makefile.inc,v 1.3 1994/12/22 04:05:39 phil Exp $

SRCS+=		netbsd-ns32k.c cpu-ns32k.c

CFLAGS+=	-DNETBSD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_ns32k_vec \
		-DSELECT_ARCHITECTURES=bfd_ns32k_arch \
		-DSELECT_VECS='&netbsd_ns32k_vec'


