#	from: @(#)Makefile	5.3 (Berkeley) 6/25/90
#	$Id: Makefile,v 1.2 1993/08/01 05:39:39 mycroft Exp $

CFLAGS+= -DBSDINETD
PROG=	uucpd
NOMAN=	noman
.ifndef EXPORTABLE_SYSTEM
DPADD+=	${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
