#	$NetBSD: Makefile,v 1.2 2026/01/21 17:57:27 christos Exp $
#

MKPROFILE=	no		# XXX hack -- should be NOPROFILE
NOLINT=		# defined
NOPICINSTALL=	# defined
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
