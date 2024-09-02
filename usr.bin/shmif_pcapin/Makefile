#	$NetBSD: Makefile,v 1.1 2024/09/02 05:14:45 ozaki-r Exp $
#

PROG=		shmif_pcapin

.include <bsd.own.mk>

SHMIFD=	${NETBSDSRCDIR}/sys/rump/net/lib/libshmif
.PATH:	${SHMIFD}

SRCS+=		shmif_pcapin.c shmif_busops.c
CPPFLAGS+=	-I${SHMIFD}

LDADD+=		-lpcap

.include <bsd.prog.mk>
