# $NetBSD: Makefile,v 1.12.2.1 2000/02/01 23:16:40 he Exp $
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
