# $NetBSD: Makefile,v 1.23 2004/05/23 02:24:52 lukem Exp $
# Original from FreeBSD, no rcs id.

MKPRIVATELIB=	yes

LIB=		install
SRCS=		file.c ftpio.c global.c lpkg.c pen.c pkgdb.c \
		plist.c str.c version.c path.c fexec.c

version.o: version.h version.c

.include <bsd.lib.mk>
