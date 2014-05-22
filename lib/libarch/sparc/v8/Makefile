#	$NetBSD: Makefile,v 1.3.4.1 2014/05/22 11:36:46 yamt Exp $

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
