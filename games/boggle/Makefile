#	$NetBSD: Makefile,v 1.16 2000/01/23 22:08:53 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

.include <bsd.own.mk>

SUBDIR=	boggle mkdict mkindex

MKDICT!=cd $(.CURDIR)/mkdict; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkdict\n" | ${MAKE} -s -f-
MKINDEX!=cd $(.CURDIR)/mkindex; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkindex\n" | ${MAKE} -s -f-
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
