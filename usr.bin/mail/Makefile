#	$NetBSD: Makefile,v 1.30 2007/05/28 12:06:28 tls Exp $
#	@(#)Makefile	8.3 (Berkeley) 4/20/95

.include <bsd.own.mk>

USE_FORT?= yes	# data-driven bugs?

USE_EDITLINE=yes
MIME_SUPPORT=yes	# currently requires USE_EDITLINE
CHARSET_SUPPORT=yes	# requires MIME_SUPPORT
THREAD_SUPPORT=yes	# EXPERIMENTAL

PROG=	mail
SRCS=	version.c support.c cmd1.c cmd2.c cmd3.c cmd4.c cmdtab.c collect.c \
	dotlock.c edit.c fio.c format.c getname.c head.c v7.local.c lex.c \
	list.c main.c names.c popen.c quit.c send.c strings.c temp.c tty.c \
	vars.c
LINKS=	${BINDIR}/mail ${BINDIR}/Mail ${BINDIR}/mail ${BINDIR}/mailx
MLINKS=	mail.1 Mail.1 mail.1 mailx.1

LDADD+=		-lutil
DPADD+=		${LIBUTIL}

.if ${USE_EDITLINE:Uno} == "yes"
SRCS+=	complete.c

CPPFLAGS+=	-DUSE_EDITLINE
LDADD+=		-ledit -ltermcap
DPADD+=		${LIBEDIT} ${LIBTERMCAP}

.if ${MIME_SUPPORT:Uno} == "yes"
SRCS+=	mime_attach.c
SRCS+=	mime_child.c
SRCS+=	mime_codecs.c
SRCS+=	mime_decode.c
SRCS+=	mime_detach.c
SRCS+=	mime_header.c

CPPFLAGS+=	-DMIME_SUPPORT
.if ${CHARSET_SUPPORT:Uno} == "yes"
CPPFLAGS+=	-DCHARSET_SUPPORT
.endif

LDADD+=		-lmagic -lz
DPADD+=		${LIBMAGIC} ${LIBZ}
.endif
.endif

.if ${THREAD_SUPPORT:Uno} == "yes"
SRCS+=	thread.c
CPPFLAGS+=	-DTHREAD_SUPPORT
.endif

WARNS=	4
CWARNFLAGS+=	-Wextra

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
