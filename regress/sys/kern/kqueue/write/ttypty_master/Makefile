# $NetBSD: Makefile,v 1.1.1.1 2002/10/03 07:46:03 jdolecek Exp $

LDADD+= -lutil
	
.include <bsd.prog.mk>

regress: ${PROG}
	./${PROG}
	@echo EVFILT_WRITE for tty test done
