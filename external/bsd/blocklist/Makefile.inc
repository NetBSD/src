#	$NetBSD: Makefile.inc,v 1.2 2020/06/15 02:29:44 christos Exp $

WARNS=6
.if !defined(LIB)
LDADD+=	-lblocklist
DPADD+= ${LIBBLOCKLIST}
.endif
CPPFLAGS+= -I${.CURDIR}/../include
CPPFLAGS+=-DHAVE_STRUCT_SOCKADDR_SA_LEN -DHAVE_UTIL_H -DHAVE_DB_H

