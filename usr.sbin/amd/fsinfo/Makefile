#	$NetBSD: Makefile,v 1.15 1999/02/24 02:58:42 itohy Exp $

PROG= fsinfo
SRCS=	fsi_gram.c fsi_lex.c wr_fstab.c wr_exportfs.c wr_dumpset.c wr_bparam.c \
wr_atab.c fsinfo.c fsi_util.c fsi_dict.c fsi_analyze.c 
MAN+= fsinfo.8
YHEADER=1
CLEANFILES+=fsi_lex.c fsi_gram.c fsi_gram.h

fsi_lex.o fsinfo.o: fsi_gram.h

.include <bsd.prog.mk>
