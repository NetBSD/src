#	@(#)Makefile	5.3 (Berkeley) 6/25/90

CFLAGS+= -DBSDINETD
PROG=	uucpd
NOMAN=	noman
.ifndef EXPORTABLE_SYSTEM
DPADD+=	${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
