#	$NetBSD: Makefile.inc,v 1.2 1997/05/07 07:15:45 mycroft Exp $

BUG_CRT_DIR=${S}/arch/${MACHINE}/stand/bugcrt

BUGCRT_DIR!=cd ${BUG_CRT_DIR}; \
     printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

BUGCRT=${BUGCRT_DIR}/bugcrt.o

$(BUGCRT):	.NOTMAIN __always_make_bugcrt
	@echo making sure the bugcrt is up to date...
	@(cd ${BUG_CRT_DIR}; ${MAKE})

__always_make_bugcrt:	.NOTMAIN
