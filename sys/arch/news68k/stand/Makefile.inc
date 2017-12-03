#	$NetBSD: Makefile.inc,v 1.9.14.2 2017/12/03 11:36:32 jdolecek Exp $

BINDIR= /usr/mdec

CFLAGS+= -ffreestanding

.include "${S}/conf/newvers_stand.mk"

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
