# $NetBSD: Makefile,v 1.11 1999/03/13 23:22:44 lukem Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
SRCS=		exec.c file.c global.c pen.c pkgdb.c plist.c str.c

MKLINT=		no
MKMAN=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
