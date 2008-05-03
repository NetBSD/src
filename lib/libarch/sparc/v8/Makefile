#	$NetBSD: Makefile,v 1.3 2008/05/03 20:43:54 nakayama Exp $

NOLINKLIB=	# defined
NOPROFILE=	# defined
NOLINT=		# defined
USE_SHLIBDIR=	yes
LIB=		sparc_v8

# Avoid linking against libc.
DPLIBC=		# nothing
LDLIBC=		-nodefaultlibs

SRCS=	sparc_v8.S

.include <bsd.lib.mk>
