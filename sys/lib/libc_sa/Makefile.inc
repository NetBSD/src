#	$NetBSD: Makefile.inc,v 1.4 1994/10/26 05:36:46 cgd Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

C_SA_DIR=	$S/lib/libc_sa

.if exists($(C_SA_DIR)/obj.${MACHINE})
C_SA_LIBDIR=	$(C_SA_DIR)/obj.${MACHINE}
.else
C_SA_LIBDIR=	$(C_SA_DIR)
.endif

C_SA_LIB=	$(C_SA_LIBDIR)/libc_sa.a

$(C_SA_LIB):	.NOTMAIN __always_make_c_sa_lib
	@echo making sure the stand-alone c library is up to date...
	@(cd $(C_SA_DIR) ; make)

__always_make_c_sa_lib: .NOTMAIN
