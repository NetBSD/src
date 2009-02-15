# $NetBSD: Makefile,v 1.3 2009/02/15 18:24:49 joerg Exp $
# Original from FreeBSD, no rcs id.

LIBISPRIVATE=	yes

LIB=		install
SRCS=		automatic.c conflicts.c decompress.c dewey.c \
		fexec.c file.c global.c gpgsig.c iterate.c lpkg.c \
		opattern.c path.c parse-config.c pkcs7.c pkg_signature.c \
		pkgdb.c pkg_io.c plist.c remove.c \
		str.c version.c var.c vulnerabilities-file.c xwrapper.c
MAN=		pkg_install.conf.5 pkg_summary.5

version.o: version.h version.c

.include <bsd.init.mk>

.PATH:	${DIST}/lib

CPPFLAGS+=	-DNETBSD
COPTS.plist.c+=	-Wno-pointer-sign

.include <bsd.lib.mk>
