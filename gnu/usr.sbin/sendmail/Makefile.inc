#	$NetBSD: Makefile.inc,v 1.5 2001/03/21 17:37:11 itojun Exp $

WARNS?=	0

CPPFLAGS+=	-DNEWDB -DNIS
CPPFLAGS+=	-DMAP_REGEX -DTCPWRAPPERS -DNETISO
CPPFLAGS+=	-DNEEDSGETIPNODE -DNETINET6
CPPFLAGS+=	-I. -I${DIST}/sendmail/sendmail -I${DIST}/sendmail/include

.if ${MKCRYPTO} != "no"
CPPFLAGS+=	-DSTARTTLS -D_FFR_TLS_TOREK
LDADD+=	-lssl -lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
