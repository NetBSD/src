# $NetBSD: Makefile,v 1.12 1999/03/22 05:02:41 hubertf Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
SRCS=		exec.c file.c global.c lpkg.c pen.c pkgdb.c plist.c str.c

MKLINT=		no
MKMAN=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
