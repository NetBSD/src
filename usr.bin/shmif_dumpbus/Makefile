#	$NetBSD: Makefile,v 1.1 2011/01/12 16:14:24 pooka Exp $
#

.include <bsd.own.mk>

SHMIFD=	${NETBSDSRCDIR}/sys/rump/net/lib/libshmif
.PATH:	${SHMIFD}

PROG=		shmif_dumpbus
SRCS+=		shmif_dumpbus.c shmif_busops.c
CPPFLAGS+=	-I${SHMIFD}

.include <bsd.prog.mk>
