# $Id: Makefile.inc,v 1.1 1994/01/28 12:38:17 pk Exp $

SRCS+=		netbsd386.c cpu-i386.c

CFLAGS+=	-DTRAD_CORE

VECTORS=	-DDEFAULT_VECTOR=netbsd_386_vec \
		-DSELECT_ARCHITECTURES=bfd_i386_arch \
		-DSELECT_VECS='&netbsd_386_vec'


