#	$NetBSD: Makefile.inc,v 1.3 1998/06/12 23:11:46 cgd Exp $

BUG_CRT_DIR=${S}/arch/mvme68k/stand/bugcrt

BUGCRT_DIR!=cd ${BUG_CRT_DIR}; \
     printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

BUGCRT=${BUGCRT_DIR}/bugcrt.o

$(BUGCRT):	.NOTMAIN __always_make_bugcrt
	@echo making sure the bugcrt is up to date...
	@(cd ${BUG_CRT_DIR}; ${MAKE})

__always_make_bugcrt:	.NOTMAIN
