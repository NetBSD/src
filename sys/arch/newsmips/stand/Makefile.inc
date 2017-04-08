#	$NetBSD: Makefile.inc,v 1.10 2017/04/08 19:53:22 christos Exp $

BINDIR= /usr/mdec
NOPIE=yes

.include "${S}/conf/newvers_stand.mk"

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
