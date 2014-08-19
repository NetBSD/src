#	$NetBSD: Makefile,v 1.3.26.1 2014/08/20 00:02:08 tls Exp $

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
