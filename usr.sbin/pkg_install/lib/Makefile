# $NetBSD: Makefile,v 1.16 2001/03/05 18:24:58 wiz Exp $
# Original from FreeBSD, no rcs id.

LIB+=		install
SRCS+=		exec.c file.c ftpio.c global.c lpkg.c pen.c pkgdb.c \
		plist.c str.c version.c

MKLINT=		no
MKMAN=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build - prevent installation of library
libinstall::

version.o: version.h version.c

.include <bsd.lib.mk>
