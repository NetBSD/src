#	$NetBSD: Makefile.inc,v 1.3.2.2 2002/02/20 23:15:55 he Exp $

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
