#	from: @(#)Makefile	5.9 (Berkeley) 7/1/91
#	$Id: Makefile,v 1.9 1997/03/24 21:57:06 christos Exp $

SUBDIR=	man1 man3 man4 man5 man7 man8 man9

FILES=makewhatis.sed man0/COPYRIGHT
FILESDIR=/usr/share/man

makedb:
	/usr/libexec/makewhatis ${DESTDIR}/usr/share/man

.include <bsd.subdir.mk>
