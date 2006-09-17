#	$NetBSD: Makefile,v 1.6 2006/09/17 03:56:03 tsutsui Exp $

HOSTPROG=	macppc_installboot
SRCS=		installboot.c cd9660.c cd9660_util.c
NOMAN=	# defined

HOST_CPPFLAGS+=	-I${.CURDIR} -I${NETBSDSRCDIR}/sys
#HOST_CPPFLAGS+=	-DDEBUG

.include <bsd.hostprog.mk>
