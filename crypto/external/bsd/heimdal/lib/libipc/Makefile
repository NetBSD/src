# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:52 elric Exp $

NOLINT=		# defined

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/ipc

LIB=		ipc

LIBDPLIBS+=	util ${NETBSDSRCDIR}/lib/libutil \
		crypt ${NETBSDSRCDIR}/lib/libcrypt

INSTALL=/usr/bin/true

SRCS= common.c client.c server.c

HOST_CPPFLAGS+=	-I${HEIMBASE}/include -DHAVE_CONFIG_H

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.lib.mk>
