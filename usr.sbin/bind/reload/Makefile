#	$NetBSD: Makefile,v 1.2 2000/01/23 21:55:44 mycroft Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:46 vixie Exp 

.PATH:	${.CURDIR}/../man

realall:	named.reload
CLEANFILES+= named.reload
MAN=	named.reload.8
FILES=	named.reload
FILESMODE= ${BINMODE}

named.reload:	named.reload.sh Makefile ${.CURDIR}/../Makefile.inc
	rm -f $@
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%INDOT%||" \
	    < ${.CURDIR}/named.reload.sh > $@
	chmod +x $@

.include "../../Makefile.inc"
.include <bsd.prog.mk>
