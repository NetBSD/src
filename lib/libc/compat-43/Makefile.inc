#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/20/91
#	$Id: Makefile.inc,v 1.9 1994/06/14 03:54:48 jtc Exp $

# compat-43 sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/compat-43 ${.CURDIR}/compat-43

SRCS+=	__setreuid.c __setregid.c creat.c getdtablesize.c gethostid.c \
	getwd.c killpg.c sethostid.c setpgrp.c setregid.c setreuid.c \
	setrgid.c setruid.c sigcompat.c

MAN3+=	creat.0 getdtablesize.0 gethostid.0 killpg.0 setregid.0 setreuid.0 \
	setruid.0 sigblock.0 sigpause.0 sigsetmask.0 sigvec.0

MLINKS+=setruid.3 setrgid.3
MLINKS+=gethostid.3 sethostid.3
