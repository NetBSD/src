#	$Header: /cvsroot/src/Makefile,v 1.6 1993/04/10 15:49:55 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR=	bin include lib libexec sbin usr.bin usr.sbin share games \
	gnulib usr.gnubin

.include <bsd.subdir.mk>
