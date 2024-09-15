#	$NetBSD: Makefile.inc,v 1.3 2024/09/15 16:06:41 christos Exp $

WARNS=4

.include <bsd.own.mk>

BINDIR?= /usr/bin

IDIST=	${NETBSDSRCDIR}/external/bsd/byacc/dist
YYPATCH != cat ${IDIST}/VERSION

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}
CPPFLAGS+= -DYYPATCH=${YYPATCH}
CWARNFLAGS+=	-Wno-missing-noreturn

.PATH: ${IDIST}
