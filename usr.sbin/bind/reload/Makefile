#	$NetBSD: Makefile,v 1.3 2003/10/19 06:12:07 lukem Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:46 vixie Exp 

.PATH:	${.CURDIR}/../man

realall:	named.reload
CLEANFILES+= named.reload
MAN=	named.reload.8
FILES=	named.reload
FILESMODE= ${BINMODE}

.include "../../Makefile.inc"

named.reload:	named.reload.sh Makefile ${.CURDIR}/../Makefile.inc
	${_MKMSGCREATE}
	${_MKCMD}\
	rm -f $@
	${_MKCMD}\
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%INDOT%||" \
	    < ${.CURDIR}/named.reload.sh > $@
	${_MKCMD}\
	chmod +x $@

.include <bsd.prog.mk>
