# $NetBSD: Makefile,v 1.1.14.2 2009/06/05 17:19:42 snj Exp $

SCRIPTS= bpm.sh
MAN= bpm.1

.include <bsd.init.mk>

.PATH:	${DIST}/bpm

CLEANFILES+=	bpm.sh

bpm.sh: ${DIST}/bpm/bpm.sh.in
	${TOOL_CAT} ${DIST}/bpm/bpm.sh.in > ${.TARGET}

.include <bsd.prog.mk>
