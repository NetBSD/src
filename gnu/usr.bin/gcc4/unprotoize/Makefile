#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:15 mrg Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=           unprotoize
SRCS=		protoize.c ${G_PROTO_OBJS:.o=.c}

CPPFLAGS+=	-I${BACKENDOBJ} \
		-DUNPROTOIZE \
		-DDEFAULT_TARGET_MACHINE=\"\" \
		-DDEFAULT_TARGET_VERSION=\"\"

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
