#	$NetBSD: Makefile.inc,v 1.2 1997/05/07 07:15:47 mycroft Exp $

LIB_SA_DIR=${S}/arch/${MACHINE}/stand/libsa

LIBSA_DIR!=cd ${LIB_SA_DIR}; \
     printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

LIBSA=${LIBSA_DIR}/libsa.a
SRTOBJ?= ${LIBSA_DIR}/SRT0.o 

$(LIBSA):	.NOTMAIN __always_make_libsa
	@echo making sure the libsa is up to date...
	@(cd ${LIB_SA_DIR}; ${MAKE})

__always_make_libsa:	.NOTMAIN
