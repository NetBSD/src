# $NetBSD: Makefile,v 1.17 2001/12/12 01:48:54 tv Exp $
# Original from FreeBSD, no rcs id.

LIB+=		install
SRCS+=		exec.c file.c ftpio.c global.c lpkg.c pen.c pkgdb.c \
		plist.c str.c version.c

NOLINT=		# defined
NOMAN=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build - prevent installation of library
libinstall::

version.o: version.h version.c

.include <bsd.lib.mk>
