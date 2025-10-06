#	$NetBSD: Makefile,v 1.1 2025/10/06 13:16:44 riastradh Exp $
#

MKPROFILE=	no		# XXX hack -- should be NOPROFILE
NOLINT=		# defined
NOMAN=		# defined
NOSTATICLIB=	# defined

LIB=		h_pthread_weak
SRCS+=		h_pthread_weak.c

LDADD+=		-latf-c

LIBDIR=		${TESTSBASE}/lib/libpthread/weak
SHLIBDIR=	${TESTSBASE}/lib/libpthread/weak
SHLIB_MAJOR=	1

LIBISCXX=	yes

.include <bsd.lib.mk>
