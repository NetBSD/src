#	$NetBSD: Makefile,v 1.11 1997/05/07 07:15:34 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

SUBDIR=	boggle mkdict mkindex

MKDICT!=cd $(.CURDIR)/mkdict; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkdict\n" | ${MAKE} -s -f-
MKINDEX!=cd $(.CURDIR)/mkindex; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkindex\n" | ${MAKE} -s -f-
WORDS=${.CURDIR}/../../share/dict/web2
FILES=dictionary dictindex
FILESDIR=/usr/share/games/boggle
CLEANFILES+=${FILES}

all: ${FILES}

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
