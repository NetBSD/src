# $NetBSD: Makefile,v 1.25 2005/11/03 21:59:55 dillo Exp $
# Original from FreeBSD, no rcs id.

MKPRIVATELIB=	yes

LIB=		install
SRCS=		automatic.c file.c ftpio.c global.c lpkg.c pen.c \
		pkgdb.c plist.c str.c version.c path.c fexec.c var.c \
		dewey.c

version.o: version.h version.c

.include <bsd.lib.mk>
