# $Id: Makefile.inc,v 1.3 1994/09/19 23:51:28 deraadt Exp $

SRCS+=		netbsd386.c cpu-i386.c

CFLAGS+=	-DNETBSD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_386_vec \
		-DSELECT_ARCHITECTURES=bfd_i386_arch \
		-DSELECT_VECS='&netbsd_386_vec'


