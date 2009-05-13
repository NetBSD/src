#	$NetBSD: Makefile,v 1.11.8.1 2009/05/13 17:18:37 jym Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

PROG=	bootblk
SRCS=		bootblk.fth
OBJS=
CLEANFILES=	assym.fth.h assym.fth.h.tmp machine sparc \
		bootblk bootblk.text bootblk.text.tmp

NOMAN=		# defined
STRIPFLAG=

USE_GENASSYM?=	no

INCLUDES=	-I. -I$S/arch -I$S -I${S}/../common/include -nostdinc
CPPFLAGS=	${INCLUDES} ${IDENT} ${PARAM} -D_LKM -D_KERNEL

#all:: bootblk.text bootblk

#clean:: 
#	rm assym.fth.h bootblk.text machine bootblk

sparc:
	ln -s ${.CURDIR}/../../../sparc/include sparc

machine:
	ln -s ${.CURDIR}/../../../${MACHINE}/include machine

assym.fth.h: genfth.cf machine sparc
	${TOOL_GENASSYM} -f -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    <${.CURDIR}/genfth.cf >assym.fth.h.tmp && \
	    mv -f assym.fth.h.tmp assym.fth.h

bootblk.text: bootblk.fth assym.fth.h
	${TOOL_AWK} '/fload/ { print "#include \"" $$2 "\"" }; !/fload/' \
	    ${.CURDIR}/bootblk.fth | /usr/bin/cpp -P >bootblk.text.tmp &&
	    mv -f bootblk.text.tmp bootblk.text

bootblk: bootblk.fth assym.fth.h
	${TOOL_FGEN} -o bootblk ${.CURDIR}/bootblk.fth

beforedepend:
	@touch .d

#
# The following are if you grab the fakeboot program from the Sun website
#

fake: bootblk bootblk.text
	../fakeboot/fakeboot -elf32 <bootblk >/bootblk
	../fakeboot/fakeboot -elf32 <bootblk.text >/bootblk.text

.include <bsd.prog.mk>
