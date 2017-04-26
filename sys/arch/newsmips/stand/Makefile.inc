#	$NetBSD: Makefile.inc,v 1.8.10.2 2017/04/26 02:53:05 pgoyette Exp $

BINDIR= /usr/mdec
NOPIE=yes

.include "${S}/conf/newvers_stand.mk"

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
