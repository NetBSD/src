# $NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:40:52 haad Exp $

SCRIPTS= bpm.sh
MAN= bpm.1

.include <bsd.init.mk>

.PATH:	${DIST}/bpm

CLEANFILES+=	bpm.sh

bpm.sh: ${DIST}/bpm/bpm.sh.in
	${TOOL_CAT} ${DIST}/bpm/bpm.sh.in > ${.TARGET}

.include <bsd.prog.mk>
