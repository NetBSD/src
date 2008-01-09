# $NetBSD: Makefile,v 1.31.2.1 2008/01/09 02:02:14 matt Exp $
# Original from FreeBSD, no rcs id.

MKPRIVATELIB=	yes

LIB=		install
SRCS=		automatic.c conflicts.c file.c ftpio.c global.c lpkg.c pen.c \
		pkgdb.c plist.c str.c version.c path.c fexec.c var.c \
		dewey.c pexec.c opattern.c iterate.c
MAN=		pkg_summary.5

version.o: version.h version.c

.include <bsd.init.mk>

.PATH:	${DIST}/lib

COPTS.plist.c+=	-Wno-pointer-sign

.include <bsd.lib.mk>
