#	$NetBSD: Makefile.inc,v 1.4 2017/01/02 17:45:26 christos Exp $

WARNS?=	1

.include <bsd.own.mk>

BINDIR?= /usr/bin

IDIST=	${NETBSDSRCDIR}/external/bsd/flex/dist/src
VERSION != ${TOOL_SED} -ne "s/PACKAGE_VERSION='\(.*\)'/\1/p" \
	${IDIST}/../configure

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST} \
	-DLOCALEDIR=\"/usr/share/locale\"

.PATH: ${IDIST}
