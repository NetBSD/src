#	$NetBSD: Makefile.inc,v 1.9 2005/01/10 02:58:58 lukem Exp $

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
LDADD+=	-lssl -lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
