#	$NetBSD: Makefile.inc,v 1.10 2005/03/09 03:11:21 christos Exp $

WARNS?=	0

CPPFLAGS+=	-DNEWDB -DDNSMAP -DIP_SRCROUTE
CPPFLAGS+=	-DMAP_REGEX -DTCPWRAPPERS -DNETISO
CPPFLAGS+=	-DNEEDSGETIPNODE
CPPFLAGS+=	-DMILTER
CPPFLAGS+=	-I. -I${.CURDIR}/../include \
		-I${DIST}/sendmail/sendmail -I${DIST}/sendmail/include

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DNETINET6
.endif

.if (${USE_YP} != "no")
CPPFLAGS+=	-DNIS
.endif

.if ${MKCRYPTO} != "no"
CPPFLAGS+=	-DSTARTTLS
LDADD+=	-lssl -lcrypto -lcrypt
DPADD+=	${LIBSSL} ${LIBCRYPTO} ${LIBCRYPT}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
