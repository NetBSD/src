#	$NetBSD: Makefile.inc,v 1.3 2016/01/09 17:38:56 christos Exp $

WARNS?=	1

.include <bsd.own.mk>

VERSION=2.6.0
BINDIR?= /usr/bin

IDIST=	${NETBSDSRCDIR}/external/bsd/flex/dist/src

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST} \
	-DLOCALEDIR=\"/usr/share/locale\"

.PATH: ${IDIST}
