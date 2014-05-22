#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 11:42:49 yamt Exp $
#

PROG=		shmif_dumpbus

.include <bsd.own.mk>

SHMIFD=	${NETBSDSRCDIR}/sys/rump/net/lib/libshmif
.PATH:	${SHMIFD}

SRCS+=		shmif_dumpbus.c shmif_busops.c
CPPFLAGS+=	-I${SHMIFD}

LDADD+=		-lpcap

.include <bsd.prog.mk>
