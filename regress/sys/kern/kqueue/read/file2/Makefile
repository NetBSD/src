# $NetBSD: Makefile,v 1.1 2002/10/12 12:02:31 jdolecek Exp $

CLEANFILES+= afile bfile

.include <bsd.prog.mk>

regress: ${PROG}
	touch afile bfile
	./${PROG} afile bfile
	@echo 'EVFILT_READ for file descriptor dup2() test done'
