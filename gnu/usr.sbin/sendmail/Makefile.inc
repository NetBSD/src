#	$NetBSD: Makefile.inc,v 1.7 2003/03/24 15:04:03 atatat Exp $

WARNS?=	0

CPPFLAGS+=	-DNEWDB -DDNSMAP -DIP_SRCROUTE
CPPFLAGS+=	-DMAP_REGEX -DTCPWRAPPERS -DNETISO
CPPFLAGS+=	-DNEEDSGETIPNODE -DNETINET6
CPPFLAGS+=	-I. -I${.CURDIR}/../include \
		-I${DIST}/sendmail/sendmail -I${DIST}/sendmail/include

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
