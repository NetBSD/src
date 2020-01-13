# $NetBSD: Makefile,v 1.2 2020/01/13 19:03:03 christos Exp $
.include <bsd.own.mk>

HOSTLIB=	install

.include <bsd.hostinit.mk>

SRCS=	automatic.c conflicts.c dewey.c \
	fexec.c file.c global.c gpgsig.c iterate.c license.c lpkg.c \
	opattern.c parse-config.c pkcs7.c pkg_signature.c \
	pkgdb.c pkg_io.c plist.c remove.c \
	str.c version.c var.c vulnerabilities-file.c xwrapper.c

HOST_CPPFLAGS+=	${CPPFLAGS}

CPPFLAGS+=      -DHAVE_NBTOOL_CONFIG_H=1 -D_FILE_OFFSET_BITS=64

.PATH:          ${NETBSDSRCDIR}/external/bsd/pkg_install/dist/lib

HOST_CPPFLAGS= ${CPPFLAGS}

#.include "${.CURDIR}/../../Makefile.nbincludes"
.include <bsd.hostlib.mk>
