#	$NetBSD: Makefile,v 1.11 1997/01/16 23:10:16 perry Exp $
#	@(#)Makefile	8.3 (Berkeley) 4/20/95

PROG=	mail
SRCS=	version.c aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c dotlock.c \
	edit.c fio.c getname.c head.c v7.local.c lex.c list.c main.c names.c \
	popen.c quit.c send.c strings.c temp.c tty.c vars.c
SFILES=	mail.help mail.tildehelp
EFILES=	mail.rc
LINKS=	${BINDIR}/mail ${BINDIR}/Mail ${BINDIR}/mail ${BINDIR}/mailx
MLINKS=	mail.1 Mail.1 mail.1 mailx.1

beforeinstall:
	cd ${.CURDIR}/misc; ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} \
	    -m 444 ${SFILES} ${DESTDIR}/usr/share/misc

.if make(install)
SUBDIR+= USD.doc
.endif

distribution:
	cd ${.CURDIR}/misc; ${INSTALL} -c -o root -g wheel \
	    -m 644 ${EFILES} ${DESTDIR}/etc

.include <bsd.prog.mk>
