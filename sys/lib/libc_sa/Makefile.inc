#	$Id: Makefile.inc,v 1.1 1993/10/13 11:16:21 cgd Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

C_SA_DIR=	$S/lib/libc_sa

.if exists($(C_SA_DIR)/obj)
C_SA_LIBDIR=	$(C_SA_DIR)/obj
.else
C_SA_LIBDIR=	$(C_SA_DIR)
.endif

C_SA_LIB=	$(C_SA_LIBDIR)/libc_sa.a

$(C_SA_LIB):	.NOTMAIN __always_make_c_sa_lib
	@echo making sure the stand-alone c library is up to date...
	@(cd $(C_SA_LIBDIR) ; make)

__always_make_c_sa_lib: .NOTMAIN
