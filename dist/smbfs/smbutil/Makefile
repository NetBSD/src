# $Id: Makefile,v 1.1.1.1 2003/02/16 08:15:08 jdolecek Exp $

PROG=	smbutil
SRCS=	smbutil.c dumptree.c login.c lookup.c view.c print.c

DPADD=	${LIBSMB}
LDADD=	-lsmb

.include <bsd.prog.mk>
