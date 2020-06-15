#	$NetBSD: Makefile.inc,v 1.1.1.1 2020/06/15 01:52:52 christos Exp $

WARNS=6
.if !defined(LIB)
LDADD+=	-lblocklist
DPADD+= ${LIBBLACKLIST}
.endif
CPPFLAGS+= -I${.CURDIR}/../include
CPPFLAGS+=-DHAVE_STRUCT_SOCKADDR_SA_LEN -DHAVE_UTIL_H -DHAVE_DB_H

