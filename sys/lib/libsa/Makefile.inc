#	$Id: Makefile.inc,v 1.1 1994/01/26 02:03:34 brezak Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

SA_DIR=	$S/lib/libsa

.if exists($(SA_DIR)/obj.${MACHINE})
SA_LIBDIR=	$(SA_DIR)/obj.${MACHINE}
.else
SA_LIBDIR=	$(SA_DIR)
.endif

SA_LIB=	$(SA_LIBDIR)/libsa.a

$(SA_LIB):	.NOTMAIN __always_make_sa_lib
	@echo making sure the stand-alone library is up to date...
	@(cd $(SA_DIR) ; make)

__always_make_sa_lib: .NOTMAIN
