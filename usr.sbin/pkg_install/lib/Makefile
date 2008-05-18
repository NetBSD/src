# $NetBSD: Makefile,v 1.33.2.1 2008/05/18 12:36:22 yamt Exp $
# Original from FreeBSD, no rcs id.

LIBISPRIVATE=	yes

LIB=		install
SRCS=		automatic.c conflicts.c decompress.c dewey.c \
		fexec.c file.c ftpio.c global.c iterate.c lpkg.c \
		opattern.c path.c pexec.c pen.c pkgdb.c plist.c \
		str.c version.c var.c vulnerabilities-file.o
MAN=		pkg_summary.5

CPPFLAGS+=	-DNETBSD

version.o: version.h version.c

.include <bsd.init.mk>

.PATH:	${DIST}/lib

CPPFLAGS+=	-DNETBSD
COPTS.plist.c+=	-Wno-pointer-sign

.include <bsd.lib.mk>
