#	from: @(#)Makefile	5.9 (Berkeley) 7/1/91
#	$Id: Makefile,v 1.7 1995/11/25 21:26:28 perry Exp $

SUBDIR=	man1 man3 man4 man5 man7 man8 man9

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 makewhatis.sed \
	    ${DESTDIR}/usr/share/man/makewhatis.sed
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 man0/COPYRIGHT \
	    ${DESTDIR}/usr/share/man/COPYRIGHT

makedb:
	/usr/libexec/makewhatis ${DESTDIR}/usr/share/man

.include <bsd.subdir.mk>
