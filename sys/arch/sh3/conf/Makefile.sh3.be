#	$NetBSD: Makefile.sh3.be,v 1.10 2001/12/09 05:00:50 atatat Exp $

# Makefile for NetBSD
#
# This makefile is constructed from a machine description:
#	config machineid
# Most changes should be made in the machine description
#	/sys/arch/sh3/conf/``machineid''
# after which you should do
#	config machineid
# Machine generic makefile changes should be made in
#	/sys/arch/sh3/conf/Makefile.sh3
# after which config should be rerun for all machines of that type.
#
# To specify debugging, add the config line: makeoptions DEBUG="-g"
# A better way is to specify -g only for a few files.
#
#	makeoptions DEBUGLIST="uvm* trap if_*"

MACHINE=	sh3
MACHINE_ARCH=	sh3
USETOOLS?=	no
NEED_OWN_INSTALL_TARGET?=no
.include <bsd.own.mk>

##
## (1) port identification
##
SH3=		$S/arch/sh3
GENASSYM=	${SH3}/sh3/genassym.cf

##
## (2) compile settings
##
CPPFLAGS+=	-Dsh3
AFLAGS+=	-x assembler-with-cpp -traditional-cpp

##
## (3) libkern and compat
##

##
## (4) local objects, compile rules, and dependencies
##
MD_OBJS=	locore.o
MD_CFILES=
MD_SFILES=	${THISSH3}/sh3/locore.s

locore.o: ${SH3}/sh3/locore.s assym.h
	${NORMAL_S}

##
## (5) link settings
##
TEXTADDR=	???
LINKFORMAT=	-Map netbsd.map -T ../../conf/sh.x
SYSTEM_LD=	@echo ${LD} ${LINKFLAGS} -o $@.out '$${SYSTEM_OBJ}' vers.o; \
		${LD} ${LINKFLAGS} -o $@.out ${SYSTEM_OBJ} vers.o
SYSTEM_LD_TAIL=
LINKFLAGS_NORMAL=
SYSTEM_LD_TAIL_DEBUG=; \
		echo mv -f $@.out $@.gdb; \
		mv -f $@.out $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@.out $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@.out $@.gdb

##
## (6) port specific target dependencies
##

# depend on CPU configuration
locore.o machdep.o: Makefile

##
## (7) misc settings
##

##
## (8) config(8) generated machinery
##
%INCLUDES

%OBJS

%CFILES

%SFILES

%LOAD

%RULES

##
## (9) port independent kernel machinery
##
.include "$S/conf/Makefile.kern.inc"
