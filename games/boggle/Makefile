#	$NetBSD: Makefile,v 1.7 1997/03/24 22:15:38 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

SUBDIR=	boggle mkdict mkindex

MKDICT!=cd $(.CURDIR)/mkdict; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkdict\n" | ${MAKE} -r -s -f - xxx
MKINDEX!=cd $(.CURDIR)/mkindex; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkindex\n" | ${MAKE} -r -s -f - xxx
WORDS=${DESTDIR}/usr/share/dict/words
FILES=dictionary dictindex
FILESDIR=/usr/share/games/boggle
CLEANFILES+=${FILES}

all: ${FILES}

${MKDICT} ${MKINDEX}: ${SUBDIR}
	

dictionary: ${MKDICT} ${WORDS}
	rm -f ${.TARGET}
	${MKDICT} < ${WORDS} > ${.TARGET}

dictindex: ${MKINDEX} dictionary
	rm -f ${.TARGET}
	${MKINDEX} < dictionary > ${.TARGET}

.include <bsd.prog.mk>
