#	$Header: /cvsroot/src/Makefile,v 1.7 1993/04/29 11:48:50 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= bin include lib libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnulib gnulibexec usr.gnubin gnugames

.include <bsd.subdir.mk>
