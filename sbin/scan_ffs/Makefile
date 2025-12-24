# $NetBSD: Makefile,v 1.12 2025/12/24 15:37:30 thorpej Exp $

.include <bsd.own.mk>

PROG=	scan_ffs

.if (${MKLFS} != "no")
LINKS=	${BINDIR}/scan_ffs ${BINDIR}/scan_lfs
MLINKS=	scan_ffs.8 scan_lfs.8
.endif

LDADD=	-lutil
DPADD=	${LIBUTIL}

SRCS=	scan_ffs.c ffs_bswap.c lfs_cksum.c
SCAN_FFS=${NETBSDSRCDIR}/sbin/scan_ffs
.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs ${NETBSDSRCDIR}/sys/ufs/lfs ${SCAN_FFS}

MAN=	scan_ffs.8

.include <bsd.prog.mk>
