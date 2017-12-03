#	$NetBSD: Makefile.inc,v 1.7.14.2 2017/12/03 11:36:33 jdolecek Exp $

BINDIR= /usr/mdec
NOPIE=yes

.include "${S}/conf/newvers_stand.mk"

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
