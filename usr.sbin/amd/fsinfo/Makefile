#	$NetBSD: Makefile,v 1.1.1.2 1997/07/24 21:24:08 christos Exp $

PROG= fsinfo
SRCS=	fsi_gram.c fsi_lex.c wr_fstab.c wr_exportfs.c wr_dumpset.c wr_bparam.c \
wr_atab.c fsinfo.c fsi_util.c fsi_dict.c fsi_analyze.c 
MAN+= fsinfo.8
CLEANFILES+=fsi_lex.c fsi_gram.c fsi_gram.h y.tab.h

fsi_lex.o: fsi_gram.h

fsi_gram.h:	y.tab.h
	@rm -f ${.TARGET}
	@cp -p ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>
