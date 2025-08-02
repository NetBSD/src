#	$NetBSD: Makefile.inc,v 1.2.56.1 2025/08/02 05:20:53 perseant Exp $

WARNS=4

.include <bsd.own.mk>

BINDIR?= /usr/bin

IDIST=	${NETBSDSRCDIR}/external/bsd/byacc/dist
YYPATCH != cat ${IDIST}/VERSION

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}
CPPFLAGS+= -DYYPATCH=${YYPATCH}
CWARNFLAGS+=	-Wno-missing-noreturn

.PATH: ${IDIST}
