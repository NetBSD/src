#	$Id: Makefile.inc,v 1.4 1993/10/23 02:33:19 cgd Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

KERNDIR=	$S/lib/libkern

.if exists($(KERNDIR)/obj.${MACHINE})
KERNLIBDIR=	$(KERNDIR)/obj.${MACHINE}
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
