#	$NetBSD: Makefile.inc,v 1.9.2.1 2017/04/21 16:53:32 bouyer Exp $

BINDIR= /usr/mdec
NOPIE=yes

.include "${S}/conf/newvers_stand.mk"

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
