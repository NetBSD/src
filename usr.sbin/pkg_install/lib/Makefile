# $NetBSD: Makefile,v 1.24 2005/11/03 21:16:41 dillo Exp $
# Original from FreeBSD, no rcs id.

MKPRIVATELIB=	yes

LIB=		install
SRCS=		automatic.c file.c ftpio.c global.c lpkg.c pen.c \
		pkgdb.c plist.c str.c version.c path.c fexec.c var.c

version.o: version.h version.c

.include <bsd.lib.mk>
