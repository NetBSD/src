#	$NetBSD: Makefile,v 1.3 2013/12/20 09:44:16 pooka Exp $
#

PROG=		shmif_dumpbus

.include <bsd.own.mk>

SHMIFD=	${NETBSDSRCDIR}/sys/rump/net/lib/libshmif
.PATH:	${SHMIFD}

SRCS+=		shmif_dumpbus.c shmif_busops.c
CPPFLAGS+=	-I${SHMIFD}

LDADD+=		-lpcap

.include <bsd.prog.mk>
