#	$NetBSD: Makefile,v 1.17 2000/12/30 17:51:26 sommerfeld Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

.include <bsd.own.mk>

SUBDIR=	boggle mkdict mkindex

MKDICTDIR!= cd $(.CURDIR)/mkdict; ${PRINTOBJDIR}
MKDICT=${MKDICTDIR}/mkdict
MKINDEXDIR!= cd $(.CURDIR)/mkindex; ${PRINTOBJDIR}
MKINDEX=${MKINDEXDIR}/mkindex
WORDS=${.CURDIR}/../../share/dict/web2
DICTFILES=${.OBJDIR}/dictionary ${.OBJDIR}/dictindex
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

${.OBJDIR}/dictionary: ${WORDS} ${MKDICT}
	rm -f ${.TARGET}
	${MKDICT} < ${WORDS} > ${.TARGET}

${.OBJDIR}/dictindex: ${.OBJDIR}/dictionary ${MKINDEX}
	rm -f ${.TARGET}
	${MKINDEX} < ${.OBJDIR}/dictionary > ${.TARGET}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
