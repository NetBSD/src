#	$NetBSD: Makefile,v 1.4 2013/10/01 17:37:08 mrg Exp $

NOLINKLIB=	# defined
NOPROFILE=	# defined
NOLINT=		# defined
USE_SHLIBDIR=	yes
LIB=		sparc_v8

# Avoid linking against libc.
DPLIBC=		# nothing
LDLIBC=		-nodefaultlibs

AFLAGS+=	-Av8

SRCS=	sparc_v8.S

.include <bsd.lib.mk>
