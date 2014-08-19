#	$NetBSD: Makefile,v 1.2.10.1 2014/08/20 00:05:04 tls Exp $
#

PROG=		shmif_dumpbus

.include <bsd.own.mk>

SHMIFD=	${NETBSDSRCDIR}/sys/rump/net/lib/libshmif
.PATH:	${SHMIFD}

SRCS+=		shmif_dumpbus.c shmif_busops.c
CPPFLAGS+=	-I${SHMIFD}

LDADD+=		-lpcap

.include <bsd.prog.mk>
