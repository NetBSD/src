#	$NetBSD: Makefile,v 1.5 2015/01/17 17:46:30 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	fsinfo

DIST=	${IDIST}/fsinfo
.PATH:	${DIST}

SRCS=	fsi_analyze.c fsi_dict.c fsi_gram.y fsi_lex.l fsi_util.c fsinfo.c \
	wr_atab.c wr_bparam.c wr_dumpset.c wr_exportfs.c wr_fstab.c
MAN+=	fsinfo.8


YPREFIX.fsi_gram.y =	fsi_
LPREFIX.fsi_lex.l =	fsi_

CWARNFLAGS.clang+=	-Wno-unneeded-internal-declaration

FSINFOOBJDIR!=cd ${.CURDIR} && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DIST} -I${FSINFOOBJDIR}
YHEADER=	1

.include <bsd.prog.mk>
