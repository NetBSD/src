#	$NetBSD: Makefile,v 1.18 2003/10/19 00:18:53 lukem Exp $

.include <bsd.own.mk>

CPPFLAGS+=	-DHAVE_CONFIG_H -I.

WARNS?=	1
PROG=	ksh
SRCS=	alloc.c c_ksh.c c_sh.c c_test.c c_ulimit.c edit.c emacs.c \
	eval.c exec.c expr.c history.c io.c jobs.c lex.c mail.c \
	main.c misc.c path.c shf.c sigact.c syn.c table.c trap.c \
	tree.c tty.c var.c version.c vi.c
DPSRCS=	emacs.out siglist.out
.if (${MKMAN} != "no")
DPSRCS+=ksh.1
.endif

# needs tbl for the man page.
USETBL= 

CLEANFILES+=	siglist.out siglist.out.tmp
# two steps to prevent the creation of a bogus siglist.out
siglist.out: config.h sh.h siglist.in siglist.sh
	${_MKMSG} " create  ${.TARGET}"
	${_MKCMD}\
	sh $(.CURDIR)/siglist.sh "$(CC) -E $(CPPFLAGS) $(DEFS) -I. -I$(.CURDIR)" < $(.CURDIR)/siglist.in > siglist.out.tmp \
	    && mv siglist.out.tmp siglist.out

# two steps to prevent the creation of a bogus emacs.out
CLEANFILES+=	emacs.out emacs.out.tmp
emacs.out: emacs.c
	${_MKMSG} " create  ${.TARGET}"
	${_MKCMD}\
	sh $(.CURDIR)/emacs-gen.sh $(.CURDIR)/emacs.c > emacs.out.tmp \
	    && mv emacs.out.tmp emacs.out

CLEANFILES+=	ksh.1 ksh.1.tmp
ksh.1: ksh.Man mkman
	${_MKMSG} " create  ${.TARGET}"
	${_MKCMD}\
	sh $(.CURDIR)/mkman ksh $(.CURDIR)/ksh.Man >ksh.1.tmp \
	    && mv ksh.1.tmp ksh.1

.include <bsd.prog.mk>
