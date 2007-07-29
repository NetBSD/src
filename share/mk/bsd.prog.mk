#	$NetBSD: bsd.prog.mk,v 1.221 2007/07/29 17:06:02 joerg Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.ifndef HOSTPROG

.include <bsd.init.mk>
.include <bsd.shlib.mk>
.include <bsd.gcc.mk>

.if defined(PROG_CXX)
PROG=	${PROG_CXX}
.endif

.if defined(PAXCTL_FLAGS)
PAXCTL_FLAGS.${PROG}?= ${PAXCTL_FLAGS}
.endif

##### Basic targets
realinstall:	proginstall scriptsinstall
clean:		cleanprog

##### PROG specific flags.
COPTS+=     ${COPTS.${PROG}}
CPPFLAGS+=  ${CPPFLAGS.${PROG}}
CXXFLAGS+=  ${CXXFLAGS.${PROG}}
OBJCOPTS+=  ${OBJCOPTS.${PROG}}
LDADD+=     ${LDADD.${PROG}}
LDFLAGS+=   ${LDFLAGS.${PROG}}
LDSTATIC+=  ${LDSTATIC.${PROG}}

##### Default values
CPPFLAGS+=	${DESTDIR:D-nostdinc ${CPPFLAG_ISYSTEM} ${DESTDIR}/usr/include}
CXXFLAGS+=	${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEMXX} ${DESTDIR}/usr/include/g++}
CFLAGS+=	${COPTS}
OBJCFLAGS+=	${OBJCOPTS}
MKDEP_SUFFIXES?=	.o .ln

# ELF platforms depend on crti.o, crtbegin.o, crtend.o, and crtn.o
.if ${OBJECT_FMT} == "ELF"
.ifndef LIBCRTBEGIN
LIBCRTBEGIN=	${DESTDIR}/usr/lib/crti.o ${_GCC_CRTBEGIN}
.MADE: ${LIBCRTBEGIN}
.endif
.ifndef LIBCRTEND
LIBCRTEND=	${_GCC_CRTEND} ${DESTDIR}/usr/lib/crtn.o
.MADE: ${LIBCRTEND}
.endif
_SHLINKER=	${SHLINKDIR}/ld.elf_so
.else
LIBCRTBEGIN?=
LIBCRTEND?=
_SHLINKER=	${SHLINKDIR}/ld.so
.endif

.ifndef LIBCRT0
LIBCRT0=	${DESTDIR}/usr/lib/crt0.o
.MADE: ${LIBCRT0}
.endif

##### Installed system library definitions
#
#	E.g.
#		LIBC?=${DESTDIR}/usr/lib/libc.a
#		LIBX11?=${DESTDIR}/usr/X11R6/lib/libX11.a
#	etc..

.for _lib in \
	archive asn1 bluetooth bsdmalloc bz2 c c_pic cdk com_err compat \
	crypt crypto crypto_idea crypto_mdc2 crypto_rc5 \
	curses dbm des edit event \
	form fl g2c gcc gnumalloc gssapi hdb intl ipsec \
	kadm5clnt kadm5srv kafs krb5 kvm l \
	m magic menu objc ossaudio pam pcap pci pmc posix pthread pthread_dbg \
	puffs radius resolv rmt roken rpcsvc rt sdp skey sl ss ssh ssl termcap \
	usbhid util wrap y z
.ifndef LIB${_lib:tu}
LIB${_lib:tu}=	${DESTDIR}/usr/lib/lib${_lib}.a
.MADE:		${LIB${_lib:tu}}	# Note: ${DESTDIR} will be expanded
.endif
.endfor

# PAM applications, if linked statically, need more libraries
.if (${MKPIC} == "no")
.if (${MKCRYPTO} != "no")
PAM_STATIC_LDADD+= -lssh
PAM_STATIC_DPADD+= ${LIBSSH}
.endif
.if (${MKKERBEROS} != "no")
PAM_STATIC_LDADD+= -lkafs -lkrb5 -lasn1 -lroken -lcom_err -lcrypto
PAM_STATIC_DPADD+= ${LIBKAFS} ${LIBKRB5} ${LIBASN1} ${LIBROKEN} \
	${LIBCOM_ERR} ${LIBCRYPTO}
.endif
.if (${MKSKEY} != "no")
PAM_STATIC_LDADD+= -lskey
PAM_STATIC_DPADD+= ${LIBSKEY}
.endif
PAM_STATIC_LDADD+= -lradius -lcrypt -lrpcsvc -lutil
PAM_STATIC_DPADD+= ${LIBRADIUS} ${LIBCRYPT} ${LIBRPCSVC} ${LIBUTIL}
.else
PAM_STATIC_LDADD=
PAM_STATIC_DPADD=
.endif

# These need + -> X transformations
.ifndef LIBSTDCXX
LIBSTDCXX=	${DESTDIR}/usr/lib/libstdc++.a
.MADE:		${LIBSTDCXX}
.endif

.ifndef LIBSUPCXX
LIBSUPCXX=	${DESTDIR}/usr/lib/libsupc++.a
.MADE:		${LIBSUPCXX}
.endif

.for _lib in \
	dps expat fntstubs fontcache fontconfig fontenc freetype FS \
	GL GLU ICE lbxutil SM X11 Xau Xaw Xdmcp Xext Xfont Xft \
	Xi Xinerama xkbfile Xmu Xmuu Xpm Xrandr Xrender Xss Xt \
	XTrap Xtst Xv Xxf86dga Xxf86misc Xxf86vm
.ifndef LIB${_lib:tu}
LIB${_lib:tu}=	${DESTDIR}/usr/X11R6/lib/lib${_lib}.a
.MADE:		${LIB${_lib:tu}}	# Note: ${DESTDIR} will be expanded
.endif
.endfor


##### Build and install rules
.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CPPFLAGS} ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CPPFLAGS} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c

.cc.o .cpp.o .cxx.o .C.o:
	${CXX} -E ${CPPFLAGS} ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.cc
	@${CXX} ${CPPFLAGS} ${CXXFLAGS} -c x.cc -o ${.TARGET}
	@rm -f x.cc
.endif

.if defined(PROG)							# {
.if defined(PROG_CXX)
SRCS?=		${PROG}.cc
.else
SRCS?=		${PROG}.c
.endif

PROGNAME?=	${PROG}

.if defined(RESCUEDIR)
CPPFLAGS+=	-DRESCUEDIR=\"${RESCUEDIR}\"
.endif

_YPSRCS=	${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}

DPSRCS+=	${_YPSRCS}
CLEANFILES+=	${_YPSRCS}

.if !empty(SRCS:N*.h:N*.sh:N*.fth)
OBJS+=		${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)					# {
.NOPATH: ${OBJS} ${PROG} ${_YPSRCS}

_PROGLDOPTS=
.if ${SHLINKDIR} != "/usr/libexec"	# XXX: change or remove if ld.so moves
.if ${OBJECT_FMT} == "ELF"
_PROGLDOPTS+=	-Wl,-dynamic-linker=${_SHLINKER}
.endif
.endif
.if ${SHLIBDIR} != "/usr/lib"
_PROGLDOPTS+=	-Wl,-rpath-link,${DESTDIR}${SHLIBDIR}:${DESTDIR}/usr/lib \
		-R${SHLIBDIR} \
		-L${DESTDIR}${SHLIBDIR}
.elif ${SHLIBINSTALLDIR} != "/usr/lib"
_PROGLDOPTS+=	-Wl,-rpath-link,${DESTDIR}${SHLIBINSTALLDIR}:${DESTDIR}/usr/lib \
		-L${DESTDIR}${SHLIBINSTALLDIR}
.endif

.if defined(PROG_CXX)
_CCLINK=	${CXX}
.else
_CCLINK=	${CC}
.endif

.if ${MKDEBUG} != "no" && ${OBJECT_FMT} == "ELF" && !commands(${PROG})
_PROGDEBUG=	${PROGNAME}.debug
.endif

.gdbinit:
	rm -f .gdbinit
.if defined(DESTDIR) && !empty(DESTDIR)
	echo "set solib-absolute-prefix ${DESTDIR}" > .gdbinit
.else
	touch .gdbinit
.endif
.for __gdbinit in ${GDBINIT}
	echo "source ${__gdbinit}" >> .gdbinit
.endfor

${OBJS} ${LOBJS}: ${DPSRCS}

${PROG}: .gdbinit ${LIBCRT0} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
.if !commands(${PROG})
	${_MKTARGET_LINK}
.if defined(DESTDIR)
	${_CCLINK} -Wl,-nostdlib \
	    ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${_PROGLDOPTS} \
	    -B${_GCC_CRTDIR}/ -B${DESTDIR}/usr/lib/  \
	    ${OBJS} ${LDADD} \
	    -L${_GCC_LIBGCCDIR} -L${DESTDIR}/usr/lib
.else
	${_CCLINK} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${_PROGLDOPTS} ${OBJS} ${LDADD}
.endif	# defined(DESTDIR)
.if defined(PAXCTL_FLAGS.${PROG})
	${PAXCTL} ${PAXCTL_FLAGS.${PROG}} ${.TARGET}
.endif
.endif	# !commands(${PROG})

${PROG}.ro: ${OBJS} ${DPADD}
	${_MKTARGET_LINK}
	${LD} -r -dc -o ${.TARGET} ${OBJS}

.if defined(_PROGDEBUG)
${_PROGDEBUG}: ${PROG}
	${_MKTARGET_CREATE}
	${OBJCOPY} --only-keep-debug ${PROG} ${_PROGDEBUG}
	${OBJCOPY} -R .gnu_debuglink --add-gnu-debuglink=${_PROGDEBUG} ${PROG} \
	    || rm -f ${_PROGDEBUG}
.endif

.endif	# defined(OBJS) && !empty(OBJS)					# }

.if !defined(MAN)
MAN=	${PROG}.1
.endif	# !defined(MAN)
.endif	# defined(PROG)							# }

realall: ${PROG} ${_PROGDEBUG} ${SCRIPTS}

cleanprog: .PHONY cleanobjs cleanextra
	rm -f a.out [Ee]rrs mklog core *.core .gdbinit ${PROG} ${_PROGDEBUG}

cleanobjs: .PHONY
.if defined(OBJS) && !empty(OBJS)
	rm -f ${OBJS} ${LOBJS}
.endif

cleanextra: .PHONY
.if defined(CLEANFILES) && !empty(CLEANFILES)
	rm -f ${CLEANFILES}
.endif

.if defined(PROG) && !target(proginstall)				# {

proginstall::	${DESTDIR}${BINDIR}/${PROGNAME} ${_PROGDEBUG:D${DESTDIR}${DEBUGDIR}${BINDIR}/${_PROGDEBUG}}
.PRECIOUS:	${DESTDIR}${BINDIR}/${PROGNAME} ${_PROGDEBUG:D${DESTDIR}${DEBUGDIR}${BINDIR}/${_PROGDEBUG}}

__proginstall: .USE
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${STRIPFLAG} ${.ALLSRC} ${.TARGET}

__progdebuginstall: .USE
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${DEBUGOWN} -g ${DEBUGGRP} -m ${DEBUGMODE} \
		${.ALLSRC} ${.TARGET}

.if ${MKUPDATE} == "no"
${DESTDIR}${BINDIR}/${PROGNAME}! ${PROG} __proginstall
.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${BINDIR}/${PROGNAME}! .MADE
.endif
.if defined(_PROGDEBUG)
${DESTDIR}${DEBUGDIR}${BINDIR}/${_PROGDEBUG}! ${_PROGDEBUG} __progdebuginstall
.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${DEBUGDIR}${BINDIR}/${_PROGDEBUG}! .MADE
.endif
.endif	#  define(_PROGDEBUG)
.else	# MKUPDATE != no
${DESTDIR}${BINDIR}/${PROGNAME}: ${PROG} __proginstall
.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${BINDIR}/${PROGNAME}: .MADE
.endif
.if defined(_PROGDEBUG)
${DESTDIR}${DEBUGDIR}${BINDIR}/${_PROGDEBUG}: ${_PROGDEBUG} __progdebuginstall
.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${DEBUGDIR}${BINDIR}/${_PROGDEBUG}: .MADE
.endif
.endif	#  defined(_PROGDEBUG)
.endif	# MKUPDATE != no

.endif	# defined(PROG) && !target(proginstall)				# }

.if !target(proginstall)
proginstall::
.endif
.PHONY:		proginstall

.if defined(SCRIPTS) && !target(scriptsinstall)				# {
SCRIPTSDIR?=${BINDIR}
SCRIPTSOWN?=${BINOWN}
SCRIPTSGRP?=${BINGRP}
SCRIPTSMODE?=${BINMODE}

scriptsinstall:: ${SCRIPTS:@S@${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}@}
.PRECIOUS: ${SCRIPTS:@S@${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}@}

__scriptinstall: .USE
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} \
	    -o ${SCRIPTSOWN_${.ALLSRC:T}:U${SCRIPTSOWN}} \
	    -g ${SCRIPTSGRP_${.ALLSRC:T}:U${SCRIPTSGRP}} \
	    -m ${SCRIPTSMODE_${.ALLSRC:T}:U${SCRIPTSMODE}} \
	    ${.ALLSRC} ${.TARGET}

.for S in ${SCRIPTS:O:u}
.if ${MKUPDATE} == "no"
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}! ${S} __scriptinstall
.if !defined(BUILD) && !make(all) && !make(${S})
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}! .MADE
.endif
.else
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}: ${S} __scriptinstall
.if !defined(BUILD) && !make(all) && !make(${S})
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}: .MADE
.endif
.endif
.endfor
.endif									# }

.if !target(scriptsinstall)
scriptsinstall::
.endif
.PHONY:		scriptsinstall

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:C/-L[  ]*/-L/Wg:M-L*} ${LOBJS} ${LDADD}
.endif

##### Pull in related .mk logic
.include <bsd.man.mk>
.include <bsd.nls.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.sys.mk>
.include <bsd.dep.mk>

${TARGETS}:	# ensure existence

.endif	# HOSTPROG
