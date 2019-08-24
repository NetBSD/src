# $NetBSD: Makefile,v 1.11 2019/08/24 13:20:52 rmind Exp $

USE_SHLIBDIR=   yes

.include <bsd.own.mk>

LIB=		npf
MAN=		libnpf.3

SRCS=		npf.c

INCS=		npf.h
INCSDIR=	/usr/include

LIBNV!=		cd ${NETBSDSRCDIR}/external/bsd/libnv/lib && ${PRINTOBJDIR}
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

DPADD+=		${LIBNV}/libnv.a
LDADD+=		-L${LIBNV} -lnv

WARNS=		5

.include <bsd.lib.mk>
