#	$NetBSD: Makefile.inc,v 1.16 2011/06/22 05:23:53 mrg Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

# Some assembler files use v9a extensions.
AFLAGS+= -Wa,-Av9a

.PATH: ${ARCHDIR}/softfloat
SRCS+= qp.c
CPPFLAGS+=	-DSOFTFLOATSPARC64_FOR_GCC -DEXCEPTIONS_WITH_SOFTFLOAT
.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
.else
#
# Can't directly use softfloat.c due to .PATH issues with the other source
# in softfloat. Just provide a wrapper which pulls it in via a #include
SRCS.softfloat= softfloat-wrapper.c
SRCS+= ${SRCS.softfloat}
CPPFLAGS+=      -I${ARCHDIR}/softfloat -I${.CURDIR}/softfloat \
		-I${.CURDIR}/softfloat/bits64

.if ${HAVE_GCC} == 45
COPTS.softfloat-wrapper.c+=	-Wno-enum-compare
.endif

.endif
