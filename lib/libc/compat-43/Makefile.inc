#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/20/91
#	$Id: Makefile.inc,v 1.5 1994/04/06 23:28:36 cgd Exp $

# compat-43 sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/compat-43 ${.CURDIR}/compat-43

SRCS+=	creat.c getwd.c killpg.c setpgrp.c setregid.c setreuid.c \
	setrgid.c setruid.c sigcompat.c

MAN2+=	creat.0 killpg.0 setregid.0 setreuid.0 setruid.0 \
	sigblock.0 sigpause.0 sigsetmask.0 sigvec.0

MLINKS+=setruid.2 setrgid.2
