#	$NetBSD: Makefile,v 1.19 2020/09/06 07:20:31 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=	3	# XXX -Wsign-compare

.include <bsd.own.mk>

PROG=	dumplfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs
MAN=	dumplfs.8
LDADD+=	-lutil
DPADD+= ${LIBUTIL}

CWARNFLAGS.gcc+=        	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
