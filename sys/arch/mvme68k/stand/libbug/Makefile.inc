#	$NetBSD: Makefile.inc,v 1.3 1997/05/07 07:15:46 mycroft Exp $

LIB_BUG_DIR=${S}/arch/${MACHINE}/stand/libbug

LIBBUG_DIR!=cd ${LIB_BUG_DIR}; \
     printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

LIBBUG=${LIBBUG_DIR}/libbug.a

$(LIBBUG):	.NOTMAIN __always_make_libbug
	@echo making sure the libbug is up to date...
	@(cd ${LIB_BUG_DIR}; ${MAKE})

__always_make_libbug:	.NOTMAIN
