#	$NetBSD: Makefile,v 1.18 2003/08/01 17:04:06 lukem Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	fsinfo

DIST=	${IDIST}/fsinfo
.PATH:	${DIST}

SRCS=	fsi_analyze.c fsi_dict.c fsi_gram.y fsi_lex.l fsi_util.c fsinfo.c \
	wr_atab.c wr_bparam.c wr_dumpset.c wr_exportfs.c wr_fstab.c
MAN+=	fsinfo.8


FSINFOOBJDIR!=cd ${.CURDIR} && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DIST} -I${FSINFOOBJDIR}
YHEADER=	1

.include <bsd.prog.mk>
