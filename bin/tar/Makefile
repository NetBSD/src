# $NetBSD: Makefile,v 1.1.8.1 2008/06/23 04:26:44 wrstuden Exp $

PROG=	bsdtar
SRCS=	bsdtar.c getdate.y matching.c read.c siginfo.c subst.c \
	tree.c util.c write.c

USE_FORT?=	yes # complex string handling

.include <bsd.own.mk>

DPSRCS+=	config_netbsd.h
CLEANFILES+=	config_netbsd.h

BSDTARDIR=	${NETBSDSRCDIR}/dist/libarchive
.PATH:		${BSDTARDIR}/tar

WARNS=	4
DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz

CPPFLAGS+=	-DPLATFORM_CONFIG_H=\"config_netbsd.h\"
CPPFLAGS+=	-I${.OBJDIR}

config_netbsd.h: ${.CURDIR}/../../lib/libarchive/config_netbsd.h
	ln -s ${.CURDIR}/../../lib/libarchive/config_netbsd.h ${.TARGET}

.include <bsd.prog.mk>
