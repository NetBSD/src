# $NetBSD: Makefile,v 1.13 2000/01/19 23:28:32 hubertf Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
SRCS=		exec.c file.c ftpio.c global.c lpkg.c pen.c pkgdb.c \
		plist.c str.c

MKLINT=		no
MKMAN=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
