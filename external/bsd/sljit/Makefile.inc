# $NetBSD: Makefile.inc,v 1.1 2012/11/05 00:23:18 alnsn Exp $

.include <bsd.own.mk>

SLJITDIST=	${NETBSDSRCDIR}/sys/external/bsd/sljit/dist
LIBSLJITDIR!=	cd ${NETBSDSRCDIR}/external/bsd/sljit/lib && ${PRINTOBJDIR}

CPPFLAGS+=	-I${SLJITDIST}/sljit_src
