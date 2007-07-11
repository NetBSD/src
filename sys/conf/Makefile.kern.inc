#	$NetBSD: Makefile.kern.inc,v 1.100.2.1 2007/07/11 20:04:52 mjf Exp $
#
# This file contains common `MI' targets and definitions and it is included
# at the bottom of each `MD' ${MACHINE}/conf/Makefile.${MACHINE}.
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
HOST_SH?=	sh
DBSYM?=		dbsym
MKDEP?=		mkdep
STRIP?=		strip
OBJCOPY?=	objcopy
OBJDUMP?=	objdump
CSCOPE?=	cscope
MKID?=		mkid
UUDECODE?=	${TOOL_UUDECODE:Uuudecode}
HEXDUMP?=	${TOOL_HEXDUMP:Uhexdump}
GENASSYM?=	${TOOL_GENASSYM:Ugenassym}
.MAKEOVERRIDES+=USETOOLS	# make sure proper value is propagated

_MKMSG?=		@\#
_MKSHMSG?=		echo
_MKSHECHO?=		echo
_MKMSG_CREATE?=		:
_MKTARGET_COMPILE?=	:
_MKTARGET_CREATE?=	:

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
INCLUDES?=	-I. ${EXTRA_INCLUDES} -I${S}/../common/include -I$S/arch \
		-I$S -nostdinc
CPPFLAGS+=	${INCLUDES} ${IDENT} ${PARAM} -D_KERNEL -D_KERNEL_OPT
DEFCOPTS?=	-O2
COPTS?=		${DEFCOPTS}
DBG=		# might contain unwanted -Ofoo
DEFWARNINGS?=	yes
.if (${DEFWARNINGS} == "yes")
. if !defined(NOGCCERROR)
CWARNFLAGS+=	-Werror
. endif
CWARNFLAGS+=	-Wall -Wno-main -Wno-format-zero-length -Wpointer-arith
CWARNFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes
CWARNFLAGS+=	-Wswitch -Wshadow
CWARNFLAGS+=	-Wcast-qual -Wwrite-strings
# Add -Wno-sign-compare.  -Wsign-compare is included in -Wall as of GCC 3.3,
# but our sources aren't up for it yet.
CWARNFLAGS+=	-Wno-sign-compare
. if ${HAVE_GCC} > 3
CWARNFLAGS+=	-Wno-pointer-sign -Wno-attributes
.  if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "sparc64" || ${MACHINE} == "prep"
CWARNFLAGS+=	-Wextra -Wno-unused-parameter
.  endif
. endif
.endif

CFLAGS+=	-ffreestanding -fno-zero-initialized-in-bss
CFLAGS+=	${CPUFLAGS} ${DEBUG} ${COPTS} ${CWARNFLAGS}
AFLAGS+=	${CPUFLAGS} -D_LOCORE -Wa,-fatal-warnings

# XXX
.if ${HAVE_GCC} > 3
CFLAGS+=	-fno-strict-aliasing
.endif

.if ${USE_FORT:Uno} == "yes"
CFLAGS+=-fstack-protector -Wstack-protector --param ssp-buffer-size=1
LDFLAGS+=-fstack-protector -Wstack-protector --param ssp-buffer-size=1
.endif

# If we want the bpendtsleep: label in kern_synch.c, we need to use
# -fno-reorder-blocks.  Don't make this a config(1) defflag without
# making sure this fragment remains valid.
.if ${HAVE_GCC} > 3 && !empty(${CFLAGS:M-DKERN_SYNCH_BPENDTSLEEP_LABEL})
CFLAGS.kern_synch.c+=	-fno-reorder-blocks
.endif

# Use the per-source COPTS variables to add -g to just those
# files that match the shell patterns given in ${DEBUGLIST}
#
.for i in ${DEBUGLIST}
. for j in ${CFILES:T:M$i.c}
COPTS.${j}+=-g
. endfor
.endfor

# compile rules: rules are named ${TYPE}_${SUFFIX} where TYPE is NORMAL or
# NOPROF and SUFFIX is the file suffix, capitalized (e.g. C for a .c file).
NORMAL_C?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"; \
		${_MKSHECHO}\
		${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c $<; \
		${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c $<
NOPROF_C?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"; \
		${_MKSHECHO}\
		${CC} ${CFLAGS} ${CPPFLAGS} -c $<; \
		${CC} ${CFLAGS} ${CPPFLAGS} -c $<
NORMAL_S?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"; \
		${_MKSHECHO}\
		${CC} ${AFLAGS} ${CPPFLAGS} -c $<; \
		${CC} ${AFLAGS} ${CPPFLAGS} -c $<

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

LIBKERNLN?=	${KERNLIBLN}

### find out what to use for libcompat
.include "$S/compat/common/Makefile.inc"
.ifndef PROF
SYSLIBCOMPAT?=	${COMPATLIB}
.else
SYSLIBCOMPAT?=	${COMPATLIB_PROF}
.endif

SYSLIBCOMPATLN?=	${COMPATLIBLN}

##
## (4) local objects, compile rules, and dependencies
##
## Each port should have a corresponding section with settings for
## MD_CFILES, MD_SFILES, and MD_OBJS, along with build rules for same.
##
MI_CFILES=devsw.c ioconf.c param.c
# the need for a MI_SFILES variable is dubitable at best
MI_OBJS=${MI_CFILES:S/.c/.o/}

param.c: $S/conf/param.c
	${_MKTARGET_CREATE}
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
SYSTEM_OBJ?=	${MD_OBJS} ${MI_OBJS} ${OBJS} ${SYSLIBCOMPAT} ${LIBKERN}
SYSTEM_DEP?=	Makefile ${SYSTEM_OBJ:O} .gdbinit
SYSTEM_LD_HEAD?=@rm -f $@
SYSTEM_LD?=	@${_MKSHMSG} "   link  ${.CURDIR:T}/${.TARGET}"; \
		${_MKSHECHO}\
		${LD} -Map $@.map --cref ${LINKFLAGS} -o $@ '$${SYSTEM_OBJ}' '$${EXTRA_OBJ}' vers.o; \
		${LD} -Map $@.map --cref ${LINKFLAGS} -o $@ ${SYSTEM_OBJ} ${EXTRA_OBJ} vers.o
SYSTEM_LD_TAIL?=@${SIZE} $@; chmod 755 $@

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
.if !empty(DEBUG:M-g*)
SYSTEM_LD_TAIL+=${SYSTEM_LD_TAIL_DEBUG}
LINKFLAGS+=	${LINKFLAGS_DEBUG}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.gdb@}
.elifndef PROF
LINKFLAGS+=	${LINKFLAGS_NORMAL}
.endif

SYSTEM_LD_TAIL+=; \
	if grep -q '^\#define.*SYMTAB_SPACE' opt_ddbparam.h; then \
		echo "${DBSYM} $@"; \
		${DBSYM} $@; \
	fi

SYSTEM_LD_HEAD+=${SYSTEM_LD_HEAD_EXTRA}
SYSTEM_LD_TAIL+=${SYSTEM_LD_TAIL_EXTRA}

##
## (6) port independent targets and dependencies: assym.h, vers.o
##
.if !target(assym.h)
assym.h: ${GENASSYM_CONF} ${GENASSYM_EXTRAS}
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} | \
	    ${GENASSYM} -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
${MD_SFILES:C/\.[Ss]/.o/} ${SFILES:C/\.[Ss]/.o/}: assym.h
.endif

.if !target(vers.o)
newvers: vers.o
vers.o: ${SYSTEM_DEP} ${SYSTEM_SWAP_DEP} $S/conf/newvers.sh $S/conf/osrelease.sh
	${_MKMSG_CREATE} vers.c
	${HOST_SH} $S/conf/newvers.sh
	${_MKTARGET_COMPILE}
	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c vers.c
.endif

.if !target(config_time.h)
EXTRA_CLEAN+= config_time.h
config_time.h: Makefile
	${_MKTARGET_CREATE}
	cp config_time.src config_time.h
.endif

.if defined(MEMORY_DISK_IMAGE)
md_root_image.h: ${MEMORY_DISK_IMAGE}
	${_MKTARGET_CREATE}
	${TOOL_HEXDUMP} -v -e '"\t" 8/1 "0x%02x, " "\n"' ${.ALLSRC} > ${.TARGET}

# XXX This is only needed when building md_root.o
CPPFLAGS+=	-DMEMORY_DISK_IMAGE
md_root.o: md_root_image.h
.endif

# depend on MEMORY_DISK_IMAGE configuration
md_root.o: Makefile

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
	${_MKMSG} "${.TARGET}ing the kernel objects"
	rm -f ${KERNELS} eddep tags *.[io] *.ln [a-z]*.s vers.c \
	    [Ee]rrs linterrs makelinks assym.h.tmp assym.h \
	    ${EXTRA_KERNELS} ${EXTRA_CLEAN}
.endif

.if !target(kernelnames)
kernelnames:
	@echo "${KERNELS} ${EXTRA_KERNELS}"
.endif

.if !target(__CLEANDEPEND)
__CLEANDEPEND: .USE
	echo .depend ${DEPS} | xargs rm -f --
.endif

# do not !target these, the kern and compat Makefiles augment them
cleandir distclean: __CLEANKERNEL __CLEANDEPEND
clean: __CLEANKERNEL
depend: .depend
dependall: depend .WAIT all

.if !target(.depend)
MKDEP_AFLAGS?=	${AFLAGS}
MKDEP_CFLAGS?=	${CFLAGS}
SSRCS=${MD_SFILES} ${SFILES}
CSRCS=${MD_CFILES} ${MI_CFILES} ${CFILES}
SRCS=${SSRCS} ${CSRCS}
DEPS=	${SRCS:T:O:u:R:S/$/.d/g}

.for _s in ${SSRCS}
${_s:T:R}.d: ${_s} assym.h config_time.h
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEP_AFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${_s:T}} ${_s}
.endfor
.for _s in ${CSRCS}
${_s:T:R}.d: ${_s} config_time.h
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEP_CFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${_s:T}} ${_s}
.endfor

assym.d: assym.h config_time.h
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} | \
	    ${GENASSYM} -- ${MKDEP} -f assym.dep -- \
	    ${CFLAGS} ${CPPFLAGS}
	sed -e 's/.*\.o:.*\.c/assym.h:/' < assym.dep >${.TARGET}
	rm -f assym.dep

.depend: ${DEPS} assym.d
	${_MKTARGET_CREATE}
	echo "${.ALLSRC}" | ${MKDEP} -D
.endif

.if !target(lint)
ALLSFILES?=	${MD_SFILES} ${SFILES}
LINTSTUBS?=	${ALLSFILES:T:R:C/^.*$/LintStub_&.c/g}
KERNLINTFLAGS?=	-bcehnxzFS
NORMAL_LN?=	${LINT} ${KERNLINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i $<

_lsrc=${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES}
LOBJS?= ${_lsrc:T:S/.c$/.ln/g} ${LIBKERNLN} ${SYSLIBCOMPATLN}

.for _sfile in ${ALLSFILES}
LintStub_${_sfile:T:R}.c: ${_sfile} assym.h
	${_MKTARGET_COMPILE}
	${CC} -E -C ${AFLAGS} ${CPPFLAGS} ${_sfile} | \
	      awk -f $S/kern/genlintstub.awk >${.TARGET}
.endfor

.for _cfile in ${CFILES} ${LINTSTUBS} ${MI_CFILES} ${MD_CFILES}
${_cfile:T:R}.ln: ${_cfile}
	${_MKTARGET_COMPILE}
	${NORMAL_LN}
.endfor

lint: ${LOBJS}
	${LINT} ${KERNLINTFLAGS} ${CPPFLAGS:M-[IDU]*} ${LOBJS}
.endif

# List of kernel images that will be installed into the root file system.
# Some platforms may need to install more than one (e.g. a netbsd.aout file
# to be loaded directly by the firmware), so this can be overriden by them.
KERNIMAGES?=	netbsd

.if !target(install)
# The install target can be redefined by putting a
# install-kernel-${MACHINE_NAME} target into /etc/mk.conf
MACHINE_NAME!=  uname -n
install: install-kernel-${MACHINE_NAME}
.if !target(install-kernel-${MACHINE_NAME})
install-kernel-${MACHINE_NAME}:
.for _K in ${KERNIMAGES}
	rm -f ${DESTDIR}/o${_K}
	ln ${DESTDIR}/${_K} ${DESTDIR}/o${_K}
	cp ${_K} ${DESTDIR}/n${_K}
	mv ${DESTDIR}/n${_K} ${DESTDIR}/${_K}
.endfor
.endif
.endif

.if !target(tags)
tags:
	@echo "see $S/kern/Makefile for tags"
.endif

EXTRA_CLEAN+= cscope.out cscope.tmp
.if !target(cscope.out)
cscope.out: Makefile depend
	${_MKTARGET_CREATE}
	@sed 's/[^:]*://;s/^ *//;s/ *\\ *$$//;' lib/kern/.depend | \
	tr -s ' ' '\n' | sed 's|^\([^.\\]\)|lib/kern/\1|;s|^../../||;' > \
	cscope.tmp
	@sed 's/[^:]*://;s/^ *//;s/ *\\ *$$//;' lib/compat/.depend | \
	tr -s ' ' '\n' | sed 's|^\([^.\\]\)|lib/compat/\1|;s|^../../||;' >> \
	cscope.tmp
	@echo ${SRCS} | cat - cscope.tmp | tr -s ' ' '\n' | sort -u | \
	${CSCOPE} -k -i - -b `echo ${INCLUDES} | sed s/-nostdinc//`
#	cscope doesn't write cscope.out if it's uptodate, so ensure
#	make doesn't keep calling cscope when not needed.
	@rm -f cscope.tmp; touch cscope.out
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
	${_MKTARGET_CREATE}
	@${MKID} `sed 's/[^:]*://;s/^ *//;s/ *\\\\ *$$//;' lib/kern/.depend \
	lib/compat/.depend | tr ' ' '\n' | sed "s|^../../||" | sort -u` \
	`sed 's/[^:]*://;s/^ *//;s/ *\\\\ *$$//;' .depend | tr ' ' '\n' \
	| sort -u`

.endif

.include "${S}/gdbscripts/Makefile.inc"

EXTRA_CLEAN+= .gdbinit
.gdbinit: Makefile ${S}/gdbscripts/Makefile.inc
	${_MKTARGET_CREATE}
	rm -f .gdbinit
.for __gdbinit in ${SYS_GDBINIT}
	echo "source ${S}/gdbscripts/${__gdbinit}" >> .gdbinit
.endfor
.if defined(GDBINIT) && !empty(GDBINIT)
.for __gdbinit in ${GDBINIT}
	echo "source ${__gdbinit}" >> .gdbinit
.endfor
.endif

# The following files use alloca(3) or variable array allocations.
# Their full name is noted as documentation.
VARSTACK=dev/cgd.c kern/uipc_socket.c miscfs/genfs/genfs_vnops.c \
    nfs/nfs_bio.c ufs/ufs/ufs_inode.c ufs/ufs/ufs_lookup.c uvm/uvm_bio.c \
    uvm/uvm_pager.c dev/ic/aic7xxx.c dev/ic/aic79xx.c arch/xen/i386/gdt.c

. if ${HAVE_GCC} > 3
.for __varstack in ${VARSTACK}
COPTS.${__varstack:T} += -Wno-stack-protector
.endfor
.endif

AFLAGS+=	${AOPTS.${.IMPSRC:T}}
CFLAGS+=	${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}}
CPPFLAGS+=	${CPPFLAGS.${.IMPSRC:T}}
CWARNFLAGS+=	${CWARNFLAGS.${.IMPSRC:T}}

##
## the end
##
