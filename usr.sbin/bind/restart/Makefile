#	$NetBSD: Makefile,v 1.3 2003/10/19 06:12:07 lukem Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:47 vixie Exp 

.PATH:	${.CURDIR}/../man

realall:	named.restart
CLEANFILES+= named.restart
MAN=	named.restart.8
FILES=	named.restart
FILESMODE= ${BINMODE}

.include "../../Makefile.inc"

named.restart:	named.restart.sh Makefile ${.CURDIR}/../Makefile.inc
	${_MKMSGCREATE}
	${_MKCMD}\
	rm -f $@
	${_MKCMD}\
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%INDOT%||" \
	    < ${.CURDIR}/named.restart.sh > $@
	${_MKCMD}\
	chmod +x $@

.include <bsd.prog.mk>
