#	$Header: /cvsroot/src/Makefile,v 1.8 1993/04/29 12:42:00 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= bin include lib libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnu

.include <bsd.subdir.mk>
