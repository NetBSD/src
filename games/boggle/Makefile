#	$NetBSD: Makefile,v 1.20 2003/10/19 01:01:44 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

.include <bsd.own.mk>

SUBDIR=	boggle mkdict mkindex

MKDICTDIR!= cd $(.CURDIR)/mkdict; ${PRINTOBJDIR}
MKDICT=${MKDICTDIR}/mkdict
MKINDEXDIR!= cd $(.CURDIR)/mkindex; ${PRINTOBJDIR}
MKINDEX=${MKINDEXDIR}/mkindex
WORDS=${NETBSDSRCDIR}/share/dict/web2
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
	${_MKMSG} " create  ${.TARGET}"
	${_MKCMD}\
	rm -f ${.TARGET}
	${_MKCMD}\
	${MKDICT} < ${WORDS} > ${.TARGET}

dictindex: dictionary ${MKINDEX}
	${_MKMSG} " create  ${.TARGET}"
	${_MKCMD}\
	rm -f ${.TARGET}
	${_MKCMD}\
	${MKINDEX} < dictionary > ${.TARGET}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
