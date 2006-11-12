#	$NetBSD: Makefile,v 1.2 2006/11/12 01:24:45 christos Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=           protoize
SRCS=		protoize.c ${G_PROTO_OBJS:.o=.c}

CPPFLAGS+=	-I${BACKENDOBJ} \
		-DDEFAULT_TARGET_MACHINE=\"\" \
		-DDEFAULT_TARGET_VERSION=\"\"


.include "../Makefile.frontend"

COPTS+=	-Wno-stack-protector

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
