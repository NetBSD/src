#	$NetBSD: Makefile,v 1.8 1997/04/01 11:42:34 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

SUBDIR=	boggle mkdict mkindex

MKDICT!=cd $(.CURDIR)/mkdict; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkdict\n" | ${MAKE} -r -s -f - xxx
MKINDEX!=cd $(.CURDIR)/mkindex; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkindex\n" | ${MAKE} -r -s -f - xxx
WORDS=${.CURDIR}/../../share/dict/web2a
FILES=dictionary dictindex
FILESDIR=/usr/share/games/boggle
CLEANFILES+=${FILES}

all: ${FILES}

${MKDICT} ${MKINDEX}: ${SUBDIR}
	

dictionary: ${WORDS}
	rm -f ${.TARGET}
	${MKDICT} < ${WORDS} > ${.TARGET}

dictindex: dictionary
	rm -f ${.TARGET}
	${MKINDEX} < dictionary > ${.TARGET}

.include <bsd.prog.mk>
