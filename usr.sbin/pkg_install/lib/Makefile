# $NetBSD: Makefile,v 1.27 2006/04/24 13:36:23 dillo Exp $
# Original from FreeBSD, no rcs id.

MKPRIVATELIB=	yes

LIB=		install
SRCS=		automatic.c file.c ftpio.c global.c lpkg.c pen.c \
		pkgdb.c plist.c str.c version.c path.c fexec.c var.c \
		dewey.c pexec.c
MAN=		pkg_summary.5

version.o: version.h version.c

.include <bsd.lib.mk>
