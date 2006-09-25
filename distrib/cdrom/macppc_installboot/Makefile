#	$NetBSD: Makefile,v 1.5.12.1 2006/09/25 21:51:39 tron Exp $

HOSTPROG=	macppc_installboot
SRCS=		installboot.c cd9660.c cd9660_util.c
NOMAN=	# defined

HOST_CPPFLAGS+=	-I${.CURDIR} -I${NETBSDSRCDIR}/sys
#HOST_CPPFLAGS+=	-DDEBUG

.include <bsd.hostprog.mk>
