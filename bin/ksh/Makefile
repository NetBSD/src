#	$NetBSD: Makefile,v 1.13 2000/06/22 05:42:22 soren Exp $

CPPFLAGS+=	-DHAVE_CONFIG_H -I.

PROG=	ksh
SRCS=	alloc.c c_ksh.c c_sh.c c_test.c c_ulimit.c edit.c emacs.c \
	eval.c exec.c expr.c history.c io.c jobs.c lex.c mail.c \
	main.c misc.c path.c shf.c sigact.c syn.c table.c trap.c \
	tree.c tty.c var.c version.c vi.c
DPSRCS=	emacs.out siglist.out ksh.1

# needs tbl for the man page.
USETBL= 

CLEANFILES+=	siglist.out emacs.out

# two steps to prevent the creation of a bogus siglist.out
siglist.out: config.h sh.h siglist.in siglist.sh
	sh $(.CURDIR)/siglist.sh "$(CPP) $(CPPFLAGS) $(DEFS) -I. -I$(.CURDIR)" < $(.CURDIR)/siglist.in > tmpsiglist.out
	mv tmpsiglist.out siglist.out

# two steps to prevent the creation of a bogus emacs.out
emacs.out: emacs.c
	sh $(.CURDIR)/emacs-gen.sh $(.CURDIR)/emacs.c > tmpemacs.out
	mv tmpemacs.out emacs.out

ksh.1: ksh.Man mkman
	sh $(.CURDIR)/mkman ksh $(.CURDIR)/ksh.Man >tmpksh.1
	mv tmpksh.1 ksh.1

emacs.o:	emacs.out
trap.o:		siglist.out

.include <bsd.prog.mk>
