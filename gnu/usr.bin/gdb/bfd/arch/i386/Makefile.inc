# $Id: Makefile.inc,v 1.2 1994/05/25 11:24:10 pk Exp $

SRCS+=		netbsd386.c cpu-i386.c

CFLAGS+=	-DNETBSD_CORE -DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_386_vec \
		-DSELECT_ARCHITECTURES=bfd_i386_arch \
		-DSELECT_VECS='&netbsd_386_vec'


