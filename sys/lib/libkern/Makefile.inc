#	$Id: Makefile.inc,v 1.6 1993/12/28 11:56:38 davidb Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

KERNDIR=	$S/lib/libkern

KERNLIBDIR!=cd $(KERNDIR); \
	printf "xxx:\n\techo \$${.OBJDIR}\n" | make -r -s -f - xxx

KERNLIB=	$(KERNLIBDIR)/libkern.a
KERNLIB_PROF=	$(KERNLIBDIR)/libkern_p.a

$(KERNLIB):		.NOTMAIN __always_make_kernlib
	@echo making sure the kern library is up to date...
	@(cd $(KERNDIR) ; $(MAKE))

$(KERNLIB_PROF):	.NOTMAIN __always_make_kernlib
	@echo making sure the profiled kern library is up to date...
	@(cd $(KERNDIR) ; $(MAKE))

__always_make_kernlib:	.NOTMAIN
