#	$NetBSD: Makefile.inc,v 1.6 2002/04/10 22:05:47 thorpej Exp $

WARNS?=	0

CPPFLAGS+=	-DNEWDB
CPPFLAGS+=	-DMAP_REGEX -DTCPWRAPPERS -DNETISO
CPPFLAGS+=	-DNEEDSGETIPNODE -DNETINET6
CPPFLAGS+=	-I. -I${DIST}/sendmail/sendmail -I${DIST}/sendmail/include

.if (${USE_YP} != "no")
CPPFLAGS+=	-DNIS
.endif

.if ${MKCRYPTO} != "no"
CPPFLAGS+=	-DSTARTTLS -D_FFR_TLS_TOREK
LDADD+=	-lssl -lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
