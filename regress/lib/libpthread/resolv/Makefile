#	$NetBSD: Makefile,v 1.1.12.1 2007/09/23 21:36:11 wrstuden Exp $

NOMAN=		# defined

PROG=		resolv

CFLAGS+=	-g

# Note: this test relies on being dynamically linked.  You will get a
# spurious PASS for a statically linked test.
DPADD+=		${LIBPTHREAD}
LDADD+=		-lpthread

regress: ${PROG}
	./${PROG} ${.CURDIR}/mach

.include <bsd.prog.mk>
