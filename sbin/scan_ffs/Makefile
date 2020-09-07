# $NetBSD: Makefile,v 1.10 2020/09/07 03:09:53 mrg Exp $

.include <bsd.own.mk>

PROG=	scan_ffs

LINKS=	${BINDIR}/scan_ffs ${BINDIR}/scan_lfs
MLINKS=	scan_ffs.8 scan_lfs.8

LDADD=	-lutil
DPADD=	${LIBUTIL}

SRCS=	scan_ffs.c lfs_cksum.c
SCAN_FFS=${NETBSDSRCDIR}/sbin/scan_ffs
.PATH:  ${NETBSDSRCDIR}/sys/ufs/lfs ${SCAN_FFS}

MAN=	scan_ffs.8

.include <bsd.prog.mk>
