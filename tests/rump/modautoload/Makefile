#	$NetBSD: Makefile,v 1.1.12.1 2014/08/20 00:04:52 tls Exp $
#

TESTSDIR=	${TESTSBASE}/rump/modautoload

TESTS_C=	t_modautoload

# Note: we link the rump kernel into the application to make this work
# on amd64.  This is the reason we keep this test in its own
# subdirectory -- otherwise the LDADD lines would get a little hairy.
LDFLAGS+=	-Wl,-E
LDADD+=		-Wl,--whole-archive	${DESTDIR}/usr/lib/librumpvfs.a	\
					${DESTDIR}/usr/lib/librump.a	\
		-Wl,--no-whole-archive
LDADD+=		-lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>
