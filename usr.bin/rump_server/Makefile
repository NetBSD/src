#	$NetBSD: Makefile,v 1.17 2021/11/01 21:37:33 nia Exp $
#

NOFULLRELRO=	yes

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver
LINT_SUPPORTED=	no	# LDADD contains -Wl,...

LDADD+= \
    -Wl,--whole-archive \
	-lrumpkern_sysproxy -lrump -lrumpvfs_nofifofs -lrumpvfs -lrumpuser \
    -Wl,--no-whole-archive
LDADD+= -lpthread

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

.include <bsd.prog.mk>
