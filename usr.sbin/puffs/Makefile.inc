#	$NetBSD: Makefile.inc,v 1.3 2008/07/29 13:17:47 pooka Exp $

USE_FORT?= yes	# network client

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.ifdef(ISRUMP)
LDADD+=		-lp2k -lukfs -lrump -lrumpuser -lpuffs -lutil -lpthread
DPADD+=		${LIBP2K} ${LIBUKFS} ${LIBRUMP} ${LIBRUMPUSER} ${LIBPUFFS}
DPADD+=		${LIBUTIL} ${LIBPTHREAD}
LDFLAGS+=	-Wl,--wrap=malloc
NOMAN=		# temporary
.endif
