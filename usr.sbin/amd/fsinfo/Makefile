#	$NetBSD: Makefile,v 1.17 2001/01/07 08:00:56 enami Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	fsinfo

DIST=	${IDIST}/fsinfo
.PATH:	${DIST}

SRCS=	fsi_analyze.c fsi_dict.c fsi_gram.c fsi_lex.c fsi_util.c fsinfo.c \
	wr_atab.c wr_bparam.c wr_dumpset.c wr_exportfs.c wr_fstab.c
MAN+=	fsinfo.8


FSINFOOBJDIR!=cd ${.CURDIR} && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DIST} -I${FSINFOOBJDIR}
YHEADER=	1
CLEANFILES+=	fsi_lex.c fsi_gram.c fsi_gram.h

fsi_lex.o fsinfo.o: fsi_gram.h

.include <bsd.prog.mk>
