#	$NetBSD: Makefile.inc,v 1.4 1998/06/12 23:11:46 cgd Exp $

LIB_BUG_DIR=${S}/arch/mvme68k/stand/libbug

LIBBUG_DIR!=cd ${LIB_BUG_DIR}; \
     printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

LIBBUG=${LIBBUG_DIR}/libbug.a

$(LIBBUG):	.NOTMAIN __always_make_libbug
	@echo making sure the libbug is up to date...
	@(cd ${LIB_BUG_DIR}; ${MAKE})

__always_make_libbug:	.NOTMAIN
