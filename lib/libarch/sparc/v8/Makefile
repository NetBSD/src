#	$NetBSD: Makefile,v 1.5 2013/10/02 15:43:02 christos Exp $

NOLINKLIB=	# defined
NOPROFILE=	# defined
NOLINT=		# defined
USE_SHLIBDIR=	yes
LIB=		sparc_v8

# Avoid linking against libc.
DPLIBC=		# nothing
LDLIBC=		-nodefaultlibs

AFLAGS+=	-Wa,-Av8

SRCS=	sparc_v8.S

.include <bsd.lib.mk>
