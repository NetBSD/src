#	$NetBSD: Makefile,v 1.9 1997/07/20 22:37:12 christos Exp $

CFLAGS+=	-DHAVE_CONFIG_H -I.

PROG=	ksh
SRCS=	alloc.c c_ksh.c c_sh.c c_test.c c_ulimit.c edit.c emacs.c \
	eval.c exec.c expr.c history.c io.c jobs.c lex.c mail.c \
	main.c misc.c path.c shf.c sigact.c syn.c table.c trap.c \
	tree.c tty.c var.c version.c vi.c
DPSRCS=	emacs.out siglist.out

# needs tbl for the man page.
USETBL= 

# two steps to prevent the creation of a bogus siglist.out
siglist.out: config.h sh.h siglist.in siglist.sh
	sh $(.CURDIR)/siglist.sh "$(CPP) $(CPPFLAGS) $(DEFS) -I. -I$(.CURDIR)" < $(.CURDIR)/siglist.in > tmpsiglist.out
	mv tmpsiglist.out siglist.out

# two steps to prevent the creation of a bogus emacs.out
emacs.out: emacs.c
	sh $(.CURDIR)/emacs-gen.sh $(.CURDIR)/emacs.c > tmpemacs.out
	mv tmpemacs.out emacs.out

.include <bsd.prog.mk>
