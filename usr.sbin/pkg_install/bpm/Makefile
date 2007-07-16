# $NetBSD: Makefile,v 1.2 2007/07/16 19:33:06 joerg Exp $

SCRIPTS= bpm.sh
MAN= bpm.1

.include <bsd.init.mk>

.PATH:	${DIST}/bpm

CLEANFILES+=	bpm.sh

bpm.sh: ${DIST}/bpm/bpm.sh.in
	${TOOL_CAT} ${DIST}/bpm/bpm.sh.in > ${.TARGET}

.include <bsd.prog.mk>
