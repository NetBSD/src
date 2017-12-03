#	$NetBSD: Makefile,v 1.13.20.2 2017/12/03 11:36:44 jdolecek Exp $

.include	<bsd.init.mk>

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

PROG=	bootblk
SRCS=		bootblk.fth
OBJS=
CLEANFILES=	ffs.fth.h lfs.fth.h assym.fth.h.tmp \
		bootblk bootblk.text bootblk.text.tmp

NOMAN=		# defined
STRIPFLAG=

USE_GENASSYM?=	no

INCLUDES=	-I. -I$S/arch -I$S -I${S}/../common/include -nostdinc
CPPFLAGS=	${INCLUDES} ${IDENT} ${PARAM} -D_LKM -D_KERNEL

ffs.fth.h: genfth.cf machine sparc
	${TOOL_GENASSYM} -f -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    ${GENASSYM_CPPFLAGS} < ${.CURDIR}/genfth.cf >ffs.fth.h.tmp && \
	    mv -f ffs.fth.h.tmp ffs.fth.h

lfs.fth.h: genlfs.cf machine sparc
	${TOOL_GENASSYM} -f -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    ${GENASSYM_CPPFLAGS} < ${.CURDIR}/genlfs.cf >lfs.fth.h.tmp && \
	    mv -f lfs.fth.h.tmp lfs.fth.h

bootblk.text: bootblk.fth ffs.fth.h lfs.fth.h
	${TOOL_AWK} '/fload/ { print "#include \"" $$2 "\"" }; !/fload/' \
	    ${.CURDIR}/bootblk.fth | /usr/bin/cpp -P >bootblk.text.tmp &&
	    mv -f bootblk.text.tmp bootblk.text

bootblk: bootblk.fth ffs.fth.h lfs.fth.h
	${TOOL_FGEN} -o bootblk ${.CURDIR}/bootblk.fth

beforedepend:
	@touch .d

#
# The following are if you grab the fakeboot program from the Sun website
#

fake: bootblk bootblk.text
	../fakeboot/fakeboot -elf32 <bootblk >/bootblk
	../fakeboot/fakeboot -elf32 <bootblk.text >/bootblk.text

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
