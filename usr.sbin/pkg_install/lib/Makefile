# $NetBSD: Makefile,v 1.19 2002/09/18 13:31:55 lukem Exp $
# Original from FreeBSD, no rcs id.

NOLINT=		# defined
NOMAN=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB+=		install
SRCS+=		exec.c file.c ftpio.c global.c lpkg.c pen.c pkgdb.c \
		plist.c str.c version.c path.c

# only needed during build - prevent installation of library
libinstall::

version.o: version.h version.c

.include <bsd.lib.mk>
