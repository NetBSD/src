#	$NetBSD: Makefile,v 1.18 2024/04/20 13:24:49 rillig Exp $
#

NOFULLRELRO=	yes

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver
NOLINT=		# LDADD contains -Wl,...

LDADD+= \
    -Wl,--whole-archive \
	-lrumpkern_sysproxy -lrump -lrumpvfs_nofifofs -lrumpvfs -lrumpuser \
    -Wl,--no-whole-archive
LDADD+= -lpthread

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

.include <bsd.prog.mk>
