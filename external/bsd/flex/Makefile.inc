#	$NetBSD: Makefile.inc,v 1.3.2.1 2017/01/07 08:53:48 pgoyette Exp $

WARNS?=	1

.include <bsd.own.mk>

BINDIR?= /usr/bin

IDIST=	${NETBSDSRCDIR}/external/bsd/flex/dist/src
.if !empty(TOOL_SED:M*/*) && !exists(${TOOL_SED})
VERSION = 3.x
.else
VERSION != ${TOOL_SED} -ne "s/PACKAGE_VERSION='\(.*\)'/\1/p" \
	${IDIST}/../configure
.endif

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST} \
	-DLOCALEDIR=\"/usr/share/locale\"

.PATH: ${IDIST}
