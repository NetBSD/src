#	$NetBSD: Makefile,v 1.1 2011/06/21 06:03:15 mrg Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=           unprotoize
SRCS=		protoize.c ${G_PROTO_OBJS:.o=.c}

CPPFLAGS+=	-I${BACKENDOBJ} \
		-DUNPROTOIZE \
		-DDEFAULT_TARGET_MACHINE=\"\" \
		-DDEFAULT_TARGET_VERSION=\"\"

.include "../Makefile.frontend"

COPTS+=	-Wno-stack-protector

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
