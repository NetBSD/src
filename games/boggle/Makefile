# $NetBSD: Makefile,v 1.3 1995/03/01 09:07:54 glass Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

SUBDIR=	boggle mkdict mkindex

MKDICT!=cd $(.CURDIR)/mkdict; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkdict\n" | ${MAKE} -r -s -f - xxx
MKINDEX!=cd $(.CURDIR)/mkindex; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkindex\n" | ${MAKE} -r -s -f - xxx

beforeinstall:
	${MKDICT} < /usr/share/dict/words > \
	    ${DESTDIR}/usr/share/games/boggle/dictionary
	${MKINDEX} < ${DESTDIR}/usr/share/games/boggle/dictionary > \
	    ${DESTDIR}/usr/share/games/boggle/dictindex
	chown ${BINOWN}.${BINGRP} ${DESTDIR}/usr/share/games/boggle/dictionary
	chmod 444 ${DESTDIR}/usr/share/games/boggle/dictionary
	chown ${BINOWN}.${BINGRP} ${DESTDIR}/usr/share/games/boggle/dictindex
	chmod 444 ${DESTDIR}/usr/share/games/boggle/dictindex

.include <bsd.subdir.mk>
