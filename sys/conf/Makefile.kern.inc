#	$NetBSD: Makefile.kern.inc,v 1.17 2002/06/18 23:46:52 wrstuden Exp $
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
DBSYM?=		dbsym
MKDEP?=		mkdep
STRIP?=		strip
OBJCOPY?=	objcopy
OBJDUMP?=	objdump
CSCOPE?=	cscope
MKID?=		mkid
.MAKEOVERRIDES+=USETOOLS	# make sure proper value is propagated

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
INCLUDES?=	-I. ${EXTRA_INCLUDES} -I$S/arch -I$S -nostdinc
CPPFLAGS+=	${INCLUDES} ${IDENT} ${PARAM} -D_KERNEL -D_KERNEL_OPT
DEFCOPTS?=	-O2
COPTS?=		${DEFCOPTS}
DBG=		# might contain unwanted -Ofoo
DEFWARNINGS?=	yes
.if (${DEFWARNINGS} == "yes")
CWARNFLAGS+=	-Werror -Wall -Wno-main -Wno-format-zero-length
WEAK_POINTERS?=	no
.if !(${WEAK_POINTERS} == "yes")
CWARNFLAGS+=	-Wpointer-arith
.endif
LOOSE_PROTOTYPES?= no
.if !(${LOOSE_PROTOTYPES} == "yes")
CWARNFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes
.endif
# XXX Delete -Wuninitialized for now, since the compiler doesn't
# XXX always get it right.  --thorpej
CWARNFLAGS+=	-Wno-uninitialized
.endif
CFLAGS+=	-ffreestanding ${DEBUG} ${COPTS} ${CWARNFLAGS}
AFLAGS+=	-D_LOCORE

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
LINKTEXT?=	${TEXTADDR:C/.+/-Ttext &/}
LINKDATA?=	${DATAADDR:C/.+/-Tdata &/}
ENTRYPOINT?=	start
LINKENTRY?=	${ENTRYPOINT:C/.+/-e &/}
LINKFLAGS?=	${LINKFORMAT} ${LINKTEXT} ${LINKDATA} ${LINKENTRY} \
		${EXTRA_LINKFLAGS}

LINKFLAGS_DEBUG?=	-X
SYSTEM_LD_TAIL_DEBUG?=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb
LINKFLAGS_NORMAL?=	-S
STRIPFLAGS?=	-g

DEBUG?=
.if ${DEBUG} == "-g"
SYSTEM_LD_TAIL+=${SYSTEM_LD_TAIL_DEBUG}
LINKFLAGS+=	${LINKFLAGS_DEBUG}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.gdb@}
.elifndef PROF
LINKFLAGS+=	${LINKFLAGS_NORMAL}
.endif

SYSTEM_LD_TAIL+=${SYSTEM_LD_TAIL_EXTRA}

##
## (6) port independent targets and dependencies: assym.h, vers.o
##
.if !target(assym.h)
assym.h: $S/kern/genassym.sh ${GENASSYM} ${GENASSYM_EXTRAS}
	cat ${GENASSYM} ${GENASSYM_EXTRAS} | \
	    sh $S/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
${MD_SFILES:C/\.[Ss]/.o/} ${SFILES:C/\.[Ss]/.o/}: assym.h
.endif

.if !target(vers.o)
newvers: vers.o
vers.o: ${SYSTEM_DEP} ${SYSTEM_SWAP_DEP}
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
## (7) misc targets: install, clean(dir), depend(all), lint, links, tags,
##                   cscope, mkid
##
## Any ports that have other stuff to be cleaned up should fill in
## EXTRA_CLEAN.  Some ports may want different settings for
## KERNLINTFLAGS, MKDEP_CFLAGS, or MKDEP_AFLAGS.
##
.if !target(__CLEANKERNEL)
__CLEANKERNEL: .USE
	@echo "${.TARGET}ing the kernel objects"
	rm -f ${KERNELS} eddep tags *.[io] [a-z]*.s vers.c \
	    [Ee]rrs linterrs makelinks assym.h.tmp assym.h \
	    ${EXTRA_KERNELS} ${EXTRA_CLEAN}
.endif

.if !target(kernelnames)
kernelnames:
	@echo "${KERNELS} ${EXTRA_KERNELS}"
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
	cat ${GENASSYM} ${GENASSYM_EXTRAS} | \
	    sh $S/kern/genassym.sh ${MKDEP} -f assym.dep ${CFLAGS} ${CPPFLAGS}
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
lint: ${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES}
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
	rm -f ${DESTDIR}/onetbsd
	ln ${DESTDIR}/netbsd ${DESTDIR}/onetbsd
	cp netbsd ${DESTDIR}/nnetbsd
	mv ${DESTDIR}/nnetbsd ${DESTDIR}/netbsd
.endif
.endif

.if !target(tags)
tags:
	@echo "see $S/kern/Makefile for tags"
.endif

EXTRA_CLEAN+= cscope.out
.if !target(cscope.out)
#
# This target is tricky. We want to include source files from the different
# kernel libraries in the output, but to get them requires calling
# make in those directories. We'd rather not call make in those directories
# if we don't have to, so we make cscope.out in two steps.
# The first step calls make on this makefile to evaluate the second
# make target. It *also* passes ` ` evaluated variables on the command
# line to the second make. These ` ` evaluated variables run make in
# the library directories, and thus provide the sources from the libs.
#
cscope.out: Makefile
	@echo Building cscope.out source database
	@${MAKE} LIBKERNSRC="`${KERNMAKE} showsources`" \
		LIBCOMPATSRC="`${COMPATMAKE} showsources`" cscope.out1

.PHONY: cscope.out1
cscope.out1:
	@echo ${SRCS} ${LIBKERNSRC:S|^|${KERNDST}/|} \
		${LIBCOMPATSRC:S|^|${COMPATDST}/|} \
		| ${CSCOPE} -k -i - -b `echo ${INCLUDES} | sed s/-nostdinc//`
#	cscope doesn't write cscope.out if it's uptodate, so ensure
#	make doesn't keep calling cscope when not needed.
	@touch cscope.out
.endif

.if !target(cscope)
cscope: cscope.out
	@${CSCOPE} -d
.endif

EXTRA_CLEAN+= ID
.if !target(mkid)
.PHONY: mkid
mkid: ID

ID: Makefile depend
# Same dance as cscope.out/cscope.out1
	@echo Building mkid database
	@${MAKE} LIBKERNSRC="`${KERNMAKE} showsources`" \
		LIBCOMPATSRC="`${COMPATMAKE} showsources`" \
		LIBKERNDEPEND="`sed 's/[^:]*://;s/^ *//;s/ *\\\\ *$$//;' lib/kern//.depend | tr ' ' '\n' | sort -u`" \
		LIBCOMPDEPEND="`sed 's/[^:]*://;s/^ *//;s/ *\\\\ *$$//;' lib/compat/.depend | tr ' ' '\n' | sort -u`" ID1

.PHONY: ID1
ID1:
	@${MKID} ${SRCS} ${LIBKERNSRC:S|^|${KERNDST}/|} \
		${LIBCOMPATSRC:S|^|${COMPATDST}/|} \
		${LIBKERNDEPEND:S|^../../||} ${LIBCOMPDEPEND:S|^../../||} \
	`sed 's/[^:]*://;s/^ *//;s/ *\\\\ *$$//;' .depend | tr ' ' '\n' | sort -u`
.endif

##
## the end
##
