#	$NetBSD: Makefile,v 1.6 2003/07/30 15:58:40 mrg Exp $

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

INCLUDES=	-I. -I$S/arch -I$S -nostdinc
CPPFLAGS=	${INCLUDES} ${IDENT} ${PARAM} -D_LKM

#all:: bootblk.text bootblk

#clean:: 
#	rm assym.fth.h bootblk.text machine bootblk

sparc:
	ln -s ${.CURDIR}/../../../sparc/include sparc

machine:
	ln -s ${.CURDIR}/../../../${MACHINE}/include machine

assym.fth.h: ${.CURDIR}/genassym.sh genfth.cf machine
	sh ${.CURDIR}/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    <${.CURDIR}/genfth.cf >assym.fth.h.tmp && \
	    mv -f assym.fth.h.tmp assym.fth.h

bootblk.text: bootblk.fth assym.fth.h
	awk '/fload/ { print "#include \"" $$2 "\"" }; !/fload/' \
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
