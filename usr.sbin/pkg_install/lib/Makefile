# $NetBSD: Makefile,v 1.9 1999/02/13 02:55:00 lukem Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
SRCS=		exec.c file.c global.c pen.c pkgdb.c plist.c str.c

# Don't need to install ftpio.3
MKMAN=		no
MKPROFILE=	no
MKPIC=		no

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
