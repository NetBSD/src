#	$NetBSD: Makefile.kern.inc,v 1.3 2001/11/16 16:15:08 atatat Exp $
#
# This file contains common `MI' targets and definitions and it is included
# at the bottom of each `MD' ${MACHINE}/conf/Makefile.${MACHINE}. There are
# many `MI' definitions that should end up in here, but they are not yet.
#
# Each target in this file should be protected with `if !target(target)'
# or `if !commands(target)' and each variable should only be conditionally
# assigned `VAR ?= VALUE', so that everything can be overriden.
#
# DEBUG is set to -g if debugging.
# PROF is set to -pg if profiling.
#
# To specify debugging, add the config line: makeoptions DEBUG="-g" 
# A better way is to specify -g only for a few files.
#
#	makeoptions DEBUGLIST="uvm* trap if_*"
#
# all ports are expected to include bsd.own.mk for toolchain settings

##
## (0) toolchain settings for things that aren't part of the standard
## toolchain
##
MKDEP?=		mkdep
STRIP?=		strip
OBJCOPY?=	objcopy
OBJDUMP?=	objdump

##
## (1) port independent source tree identification
##
# source tree is located via $S relative to the compilation directory
.ifndef S
S!=	cd ../../../..; pwd
.endif

##
## (2) compile settings
##
## CPPFLAGS, CFLAGS, and AFLAGS must be set in the port's Makefile
##
INCLUDES?=	-I. ${MD_INCLUDES} -I$S/arch -I$S -nostdinc
CWARNFLAGS?=	-Werror -Wall -Wmissing-prototypes -Wstrict-prototypes \
		-Wpointer-arith
# XXX Delete -Wuninitialized for now, since the compiler doesn't
# XXX always get it right.  --thorpej
CWARNFLAGS+=	-Wno-uninitialized

.if !defined(HAVE_EGCS)
HAVE_EGCS!=	${CC} --version | egrep "^(2\.[89]|egcs)" ; echo
.endif
.if (${HAVE_EGCS} != "")
CWARNFLAGS+=	-Wno-main
.endif

# Define a set of xxx_G variables that will add -g to just those
# files that match the shell patterns given in ${DEBUGLIST}
#
.for i in ${DEBUGLIST}
.for j in ${CFILES:T:M$i.c}
${j:R}_G?= -g
.endfor
.endfor

# compile rules: rules are named ${TYPE}_${SUFFIX} where TYPE is NORMAL or
# NOPROF and SUFFIX is the file suffix, capitalized (e.g. C for a .c file).
NORMAL_C?=	${CC} ${CFLAGS} ${CPPFLAGS} ${${<:T:R}_G} ${PROF} -c $<
NOPROF_C?=	${CC} ${CFLAGS} ${CPPFLAGS} ${${<:T:R}_G} -c $<
NORMAL_S?=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<

##
## (3) libkern and compat 
##
## Set KERN_AS in the port Makefile to "obj" or "library".  The
## default is "library", as documented in $S/lib/libkern/Makefile.inc.
##

### find out what to use for libkern
.include "$S/lib/libkern/Makefile.inc"
.ifndef PROF
LIBKERN?=	${KERNLIB}
.else
LIBKERN?=	${KERNLIB_PROF}
.endif

### find out what to use for libcompat
.include "$S/compat/common/Makefile.inc"
.ifndef PROF
LIBCOMPAT?=	${COMPATLIB}
.else
LIBCOMPAT?=	${COMPATLIB_PROF}
.endif

##
## (4) local objects, compile rules, and dependencies
##
## Each port should have a corresponding section with settings for
## MD_CFILES, MD_SFILES, and MD_OBJS, along with build rules for same.
##
MI_CFILES=ioconf.c param.c
# the need for a MI_SFILES variable is dubitable at best
MI_OBJS=${MI_CFILES:S/.c/.o/}

param.c: $S/conf/param.c
	rm -f param.c
	cp $S/conf/param.c .

param.o: Makefile

.for _cfile in ${MI_CFILES}
${_cfile:T:R}.o: ${_cfile}
	${NORMAL_C}
.endfor

##
## (5) link settings
##
## TEXTADDR (or LOADADDRESS), LINKFORMAT, and any EXTRA_LINKFLAGS must
## be set in the port's Makefile.  The port specific definitions for
## LINKFLAGS_NORMAL and LINKFLAGS_DEBUG will added to the LINKFLAGS
## depending on the value of DEBUG.
##
# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ?=	${MD_OBJS} ${MI_OBJS} ${OBJS} ${LIBCOMPAT} ${LIBKERN}
SYSTEM_DEP?=	Makefile ${SYSTEM_OBJ}
SYSTEM_LD_HEAD?=	@rm -f $@
SYSTEM_LD?=	@echo ${LD} ${LINKFLAGS} -o $@ '$${SYSTEM_OBJ}' vers.o; \
		${LD} ${LINKFLAGS} -o $@ ${SYSTEM_OBJ} vers.o
SYSTEM_LD_TAIL?=	@${SIZE} $@; chmod 755 $@

TEXTADDR?=	${LOADADDRESS}			# backwards compatibility
LINKTEXT?=	-Ttext ${TEXTADDR}
LINKDATA?=	${DATAADDR:D-Tdata ${DATAADDR}}
ENTRYPOINT?=	start
LINKENTRY?=	-e ${ENTRYPOINT}
LINKFLAGS?=	${LINKFORMAT} ${LINKTEXT} ${LINKDATA} ${LINKENTRY} \
		${EXTRA_LINKFLAGS}

LINKFLAGS_DEBUG?=
SYSTEM_LD_TAIL_DEBUG?=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb
LINKFLAGS_NORMAL?=
STRIPFLAGS?=	-g

DEBUG?=
.if ${DEBUG} == "-g"
SYSTEM_LD_TAIL+=${SYSTEM_LD_TAIL_DEBUG}
LINKFLAGS+=	${LINKFLAGS_DEBUG}
.else
LINKFLAGS+=	${LINKFLAGS_NORMAL}
.endif

##
## (6) port independent targets and dependencies: assym.h, newvers
##
.if !target(assym.h)
assym.h: $S/kern/genassym.sh ${GENASSYM}
	sh $S/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    < ${GENASSYM} > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
.endif

.if !target(newvers)
newvers: ${SYSTEM_DEP} ${SYSTEM_SWAP_DEP}
	sh $S/conf/newvers.sh
	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c vers.c
.endif

# depend on root or device configuration
autoconf.o conf.o: Makefile

# depend on network or filesystem configuration
uipc_proto.o vfs_conf.o: Makefile

# depend on maxusers and CPU configuration
assym.h machdep.o: Makefile

##
## (7) misc targets: install, clean(dir), depend(all), lint, links, tags
##
## Any ports that have other stuff to be cleaned up should fill in
## EXTRA_CLEAN.  Some ports may want different settings for
## KERNLINTFLAGS, MKDEP_CFLAGS, or MKDEP_AFLAGS.
##
.if !target(__CLEANKERNEL)
__CLEANKERNEL: .USE
	@echo "${.TARGET}ing the kernel objects"
	rm -f eddep *netbsd netbsd.gdb tags *.[io] [a-z]*.s \
	    [Ee]rrs linterrs makelinks assym.h.tmp assym.h \
	    ${EXTRA_CLEAN}
.endif

.if !target(__CLEANDEPEND)
__CLEANDEPEND: .USE
	rm -f .depend
.endif

# do not !target these, the kern and compat Makefiles augment them
cleandir distclean: __CLEANKERNEL __CLEANDEPEND
clean: __CLEANKERNEL
depend: .depend

.if !target(.depend)
SRCS?=		${MD_SFILES} ${MD_CFILES} ${MI_CFILES} ${CFILES} ${SFILES}
MKDEP_AFLAGS?=	${AFLAGS}
MKDEP_CFLAGS?=	${CFLAGS}
.depend: ${SRCS} assym.h
	${MKDEP} ${MKDEP_AFLAGS} ${CPPFLAGS} ${MD_SFILES} ${SFILES}
	${MKDEP} -a ${MKDEP_CFLAGS} ${CPPFLAGS} ${MD_CFILES} ${MI_CFILES} \
	    ${CFILES}
	sh $S/kern/genassym.sh ${MKDEP} -f assym.dep ${CFLAGS} \
	    ${CPPFLAGS} < ${GENASSYM}
	@sed -e 's/.*\.o:.*\.c/assym.h:/' < assym.dep >> .depend
	@rm -f assym.dep
.endif

.if !target(dependall)
dependall: depend all
.endif

.if !target(lint)
ALLSFILES?=	${MD_SFILES} ${SFILES}
LINTSTUBS?=	${ALLSFILES:T:R:C/^.*$/LintStub_&.c/g}
KERNLINTFLAGS?=	-hbxncez -Dvolatile=
.for _sfile in ${ALLSFILES}
LintStub_${_sfile:T:R}.c: ${_sfile} assym.h
	${CC} -E -C ${AFLAGS} ${CPPFLAGS} ${_sfile} | \
	      awk -f $S/kern/genlintstub.awk >${.TARGET}
.endfor
lint: ${CFILES} ${KERNLINTSTUBS} ${MI_CFILES} ${MD_CFILES}
	@${LINT} ${KERNLINTFLAGS} ${CPPFLAGS} -UKGDB \
	    ${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES} | \
	    grep -v 'static function .* unused'
.endif

.if !target(install)
# The install target can be redefined by putting a
# install-kernel-${MACHINE_NAME} target into /etc/mk.conf
MACHINE_NAME!=  uname -n
install: install-kernel-${MACHINE_NAME}
.if !target(install-kernel-${MACHINE_NAME}})
install-kernel-${MACHINE_NAME}:
	rm -f /onetbsd
	ln /netbsd /onetbsd
	cp netbsd /nnetbsd
	mv /nnetbsd /netbsd
.endif
.endif

.if !target(tags)
tags:
	@echo "see $S/kern/Makefile for tags"
.endif

##
## the end
##
