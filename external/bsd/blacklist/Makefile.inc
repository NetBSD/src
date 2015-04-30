#	$NetBSD: Makefile.inc,v 1.3.2.2 2015/04/30 06:07:33 riz Exp $

WARNS=6
.if !defined(LIB)
LDADD+=	-lblacklist
DPADD+= ${LIBBLACKLIST}
.endif
CPPFLAGS+= -I${.CURDIR}/../include
CPPFLAGS+=-DHAVE_STRUCT_SOCKADDR_SA_LEN -DHAVE_UTIL_H -DHAVE_DB_H

