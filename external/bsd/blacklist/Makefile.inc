#	$NetBSD: Makefile.inc,v 1.1 2015/01/21 16:16:00 christos Exp $

WARNS=6
LDADD+=	-lblacklist
DPADD+= ${LIBBLACKLIST}
CPPFLAGS+= -I${.CURDIR}/../include
