#	$NetBSD: Makefile,v 1.36.22.1 2018/06/25 07:26:10 pgoyette Exp $
#	@(#)Makefile	8.3 (Berkeley) 4/20/95

.include <bsd.own.mk>

USE_FORT?= yes	# data-driven bugs?

USE_EDITLINE?=yes
MIME_SUPPORT?=yes	# currently requires USE_EDITLINE
CHARSET_SUPPORT?=yes	# requires MIME_SUPPORT
THREAD_SUPPORT?=yes	# EXPERIMENTAL

# Work around some problems in -current.
# See the source code for more info.
#
CPPFLAGS+=	-DBROKEN_EXEC_TTY_RESTORE	# broken since 4.99.10
CPPFLAGS+=	-DBROKEN_CLONE_STAT		# see PRs 37878 and 37550
CPPFLAGS+=	-DBROKEN_MAGIC			# bad MIME type on short files

# Debugging options (most should go away - please leave for now).
#
#CPPFLAGS+=	-DDEBUG_FILE_LEAK

PROG=	mail
SRCS=	version.c support.c cmd1.c cmd2.c cmd3.c cmd4.c cmdtab.c collect.c \
	dotlock.c edit.c fio.c format.c getname.c head.c v7.local.c lex.c \
	list.c main.c names.c popen.c quit.c send.c sig.c strings.c temp.c \
	tty.c vars.c
LINKS=	${BINDIR}/mail ${BINDIR}/mailx
MLINKS=	mail.1 mailx.1

LDADD+=		-lutil
DPADD+=		${LIBUTIL}

.if ${USE_EDITLINE:Uno} == "yes"
SRCS+=	complete.c

CPPFLAGS+=	-DUSE_EDITLINE
LDADD+=		-ledit -lterminfo
DPADD+=		${LIBEDIT} ${LIBTERMINFO}

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

CWARNFLAGS+=	-Wextra

.PATH: ${.CURDIR}/misc

.if ${MKSHARE} != "no"
FILESDIR=		/usr/share/misc
FILES=			mail.help mail.tildehelp
.endif

SUBDIR.roff+= USD.doc

CONFIGFILES=		mail.rc
FILESDIR_mail.rc=	/etc

COPTS.format.c += -Wno-format-nonliteral

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
