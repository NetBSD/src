# $NetBSD: Makefile,v 1.8 1999/01/19 17:02:01 hubertf Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
SRCS=		exec.c file.c global.c pen.c pkgdb.c plist.c str.c

# Don't need to install ftpio.3
NOMAN=		yes
NOPROFILE=	yes
NOPIC=		yes

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
