#	$NetBSD: Makefile,v 1.4 2003/10/21 10:01:22 lukem Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:46 vixie Exp 

.PATH:	${.CURDIR}/../man

realall:	named.reload
CLEANFILES+= named.reload
MAN=	named.reload.8
FILES=	named.reload
FILESMODE= ${BINMODE}

.include "../../Makefile.inc"

named.reload:	named.reload.sh Makefile ${.CURDIR}/../Makefile.inc
	${_MKTARGET_CREATE}
	rm -f $@
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%INDOT%||" \
	    < ${.CURDIR}/named.reload.sh > $@
	chmod +x $@

.include <bsd.prog.mk>
