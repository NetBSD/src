# $NetBSD: Makefile,v 1.5.2.2 1998/11/06 20:41:18 cgd Exp $
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
