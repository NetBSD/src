#	$Id: Makefile.inc,v 1.3 1993/10/13 12:19:37 cgd Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

KERNDIR=	$S/lib/libkern

.if exists($(KERNDIR)/obj)
KERNLIBDIR=	$(KERNDIR)/obj
.else
KERNLIBDIR=	$(KERNDIR)
.endif

KERNLIB=	$(KERNLIBDIR)/libkern.a
KERNLIB_PROF=	$(KERNLIBDIR)/libkern_p.a

$(KERNLIB):		.NOTMAIN __always_make_kernlib
	@echo making sure the kern library is up to date...
	@(cd $(KERNDIR) ; make)

$(KERNLIB_PROF):	.NOTMAIN __always_make_kernlib
	@echo making sure the profiled kern library is up to date...
	@(cd $(KERNDIR) ; make)

__always_make_kernlib:	.NOTMAIN
