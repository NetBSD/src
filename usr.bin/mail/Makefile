#	$NetBSD: Makefile,v 1.16 2000/07/05 03:30:07 enami Exp $
#	@(#)Makefile	8.3 (Berkeley) 4/20/95

.include <bsd.own.mk>

PROG=	mail
SRCS=	version.c aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c dotlock.c \
	edit.c fio.c getname.c head.c v7.local.c lex.c list.c main.c names.c \
	popen.c quit.c send.c strings.c temp.c tty.c vars.c
EFILES=	mail.rc
LINKS=	${BINDIR}/mail ${BINDIR}/Mail ${BINDIR}/mail ${BINDIR}/mailx
MLINKS=	mail.1 Mail.1 mail.1 mailx.1

.if ${MKSHARE} != "no"
FILESDIR=/usr/share/misc
FILES=	${.CURDIR}/misc/mail.help ${.CURDIR}/misc/mail.tildehelp

.if make(install)
SUBDIR+= USD.doc
.endif
.endif

distribution:
	cd ${.CURDIR}/misc; ${INSTALL} ${INSTPRIV} -c -o root -g wheel \
	    -m 644 ${EFILES} ${DESTDIR}/etc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
