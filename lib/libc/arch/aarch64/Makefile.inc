# $NetBSD: Makefile.inc,v 1.3.4.1 2026/11/04 23:19:11 perseant Exp $

SRCS+=	__sigtramp2.S
#SRCS+= qdivrem.c

CPPFLAGS += -I.

.PATH: ${ARCHDIR}/softfloat
SRCS+=	qp.c
CPPFLAGS+=	-DSOFTFLOATAARCH64_FOR_GCC -DEXCEPTIONS_WITH_SOFTFLOAT
CPPFLAGS+=	-DSOFTFLOAT_NEED_FIXUNS
#
# Can't directly use softfloat.c due to .PATH issues with the other source
# in softfloat. Just provide a wrapper which pulls it in via a #include
SRCS.softfloat= softfloat-wrapper.c
SRCS+= ${SRCS.softfloat}
CPPFLAGS+=	-I${ARCHDIR}/softfloat -I${LIBCDIR}/softfloat \
		-I${LIBCDIR}/softfloat/bits64
