# $NetBSD: Makefile,v 1.1.2.2 2008/03/23 00:11:39 matt Exp $

PROG=	bsdcpio
SRCS=	cmdline.c cpio.c err.c matching.c pathmatch.c

USE_FORT?=	yes # complex string handling

.include <bsd.own.mk>

DPSRCS+=	config_netbsd.h
CLEANFILES+=	config_netbsd.h

BSDCPIODIR=	${NETBSDSRCDIR}/dist/libarchive
.PATH:		${BSDCPIODIR}/cpio

WARNS=	4
DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz

CPPFLAGS+=	-DPLATFORM_CONFIG_H=\"config_netbsd.h\"
CPPFLAGS+=	-I${.OBJDIR}

config_netbsd.h: ${.CURDIR}/../../lib/libarchive/config_netbsd.h
	ln -s ${.CURDIR}/../../lib/libarchive/config_netbsd.h ${.TARGET}

.include <bsd.prog.mk>
