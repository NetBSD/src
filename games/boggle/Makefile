#	$NetBSD: Makefile,v 1.18 2002/03/05 19:17:44 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

.include <bsd.own.mk>

SUBDIR=	boggle mkdict mkindex

MKDICTDIR!= cd $(.CURDIR)/mkdict; ${PRINTOBJDIR}
MKDICT=${MKDICTDIR}/mkdict
MKINDEXDIR!= cd $(.CURDIR)/mkindex; ${PRINTOBJDIR}
MKINDEX=${MKINDEXDIR}/mkindex
WORDS=${.CURDIR}/../../share/dict/web2
DICTFILES=dictionary dictindex
.if ${MKSHARE} != "no"
FILES=${DICTFILES}
FILESDIR=/usr/share/games/boggle
.endif
CLEANFILES+=${DICTFILES}

realall: ${FILES}

${MKDICT}:
	@cd ${.CURDIR}/mkdict && ${MAKE}

${MKINDEX}:
	@cd ${.CURDIR}/mkindex && ${MAKE}

dictionary: ${WORDS} ${MKDICT}
	rm -f ${.TARGET}
	${MKDICT} < ${WORDS} > ${.TARGET}

dictindex: dictionary ${MKINDEX}
	rm -f ${.TARGET}
	${MKINDEX} < dictionary > ${.TARGET}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
