# $NetBSD: Makefile.inc,v 1.1.4.2 2013/01/16 05:28:00 yamt Exp $

.include <bsd.own.mk>

SLJITDIST=	${NETBSDSRCDIR}/sys/external/bsd/sljit/dist
LIBSLJITDIR!=	cd ${NETBSDSRCDIR}/external/bsd/sljit/lib && ${PRINTOBJDIR}

CPPFLAGS+=	-I${SLJITDIST}/sljit_src
