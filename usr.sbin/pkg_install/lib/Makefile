# $NetBSD: Makefile,v 1.7 1998/10/08 12:15:24 agc Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
SRCS=		exec.c file.c global.c pen.c plist.c str.c

# Don't need to install ftpio.3
NOMAN=		yes
NOPROFILE=	yes
NOPIC=		yes

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
