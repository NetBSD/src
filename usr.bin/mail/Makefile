#	$NetBSD: Makefile,v 1.21 2004/05/16 09:53:10 lukem Exp $
#	@(#)Makefile	8.3 (Berkeley) 4/20/95

.include <bsd.own.mk>

PROG=	mail
SRCS=	version.c support.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c dotlock.c \
	edit.c fio.c getname.c head.c v7.local.c lex.c list.c main.c names.c \
	popen.c quit.c send.c strings.c temp.c tty.c vars.c
LINKS=	${BINDIR}/mail ${BINDIR}/Mail ${BINDIR}/mail ${BINDIR}/mailx
MLINKS=	mail.1 Mail.1 mail.1 mailx.1

WARNS=	2

.PATH: ${.CURDIR}/misc

.if ${MKSHARE} != "no"
FILESDIR=		/usr/share/misc
FILES=			mail.help mail.tildehelp

.if make(install)
SUBDIR+= USD.doc
.endif
.endif

CONFIGFILES=		mail.rc
FILESDIR_mail.rc=	/etc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
