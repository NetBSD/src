#	$Id: Makefile.inc,v 1.7 1994/01/09 17:22:54 ws Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

KERNDIR=	$S/lib/libkern

KERNLIBDIR!=cd $(KERNDIR); \
	printf "xxx:\n\techo \$${.OBJDIR}\n" | $(MAKE) -r -s -f - xxx

KERNLIB=	$(KERNLIBDIR)/libkern.a
KERNLIB_PROF=	$(KERNLIBDIR)/libkern_p.a

$(KERNLIB):		.NOTMAIN __always_make_kernlib
	@echo making sure the kern library is up to date...
	@(cd $(KERNDIR) ; $(MAKE))

$(KERNLIB_PROF):	.NOTMAIN __always_make_kernlib
	@echo making sure the profiled kern library is up to date...
	@(cd $(KERNDIR) ; $(MAKE))

__always_make_kernlib:	.NOTMAIN
