#	$NetBSD: Makefile,v 1.10 1997/04/30 20:07:40 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

SUBDIR=	boggle mkdict mkindex

MKDICT!=cd $(.CURDIR)/mkdict; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkdict\n" | \
	${MAKE} -r -s -f - xxx | grep mkdict
MKINDEX!=cd $(.CURDIR)/mkindex; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkindex\n" | \
	${MAKE} -r -s -f - xxx | grep mkindex
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
