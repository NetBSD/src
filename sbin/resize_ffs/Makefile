#	$NetBSD: Makefile,v 1.3 2011/01/05 02:18:15 riz Exp $

.include <bsd.own.mk>

WARNS?=	3	# XXX: sign-compare issues

PROG=resize_ffs
MAN=resize_ffs.8
SRCS=resize_ffs.c ffs_bswap.c

.PATH: ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
