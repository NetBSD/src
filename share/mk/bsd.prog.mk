#	$NetBSD: bsd.prog.mk,v 1.151 2002/02/03 21:40:14 christos Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.include <bsd.init.mk>

##### Basic targets
.PHONY:		cleanextra cleanobjs cleanprog proginstall scriptsinstall
realinstall:	proginstall scriptsinstall
clean:		cleanprog

##### Default values
CPPFLAGS+=	${DESTDIR:D-nostdinc ${CPPFLAG_ISYSTEM} ${DESTDIR}/usr/include}
CXXFLAGS+=	${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEM} ${DESTDIR}/usr/include/g++}
CFLAGS+=	${COPTS}

# ELF platforms depend on crtbegin.o and crtend.o
.if ${OBJECT_FMT} == "ELF"
LIBCRTBEGIN?=	${DESTDIR}/usr/lib/crtbegin.o
.NOSUFF: ${LIBCRTBEGIN}
LIBCRTEND?=	${DESTDIR}/usr/lib/crtend.o
.NOSUFF: ${LIBCRTEND}
_SHLINKER=	${SHLINKDIR}/ld.elf_so
.else
LIBCRTBEGIN?=
LIBCRTEND?=
_SHLINKER=	${SHLINKDIR}/ld.so
.endif

LIBCRT0?=	${DESTDIR}/usr/lib/crt0.o
.NOSUFF: ${LIBCRT0}

LIBBZ2?=	${DESTDIR}/usr/lib/libbz2.a
.NOSUFF: ${LIBBZ2}
LIBC?=		${DESTDIR}/usr/lib/libc.a
.NOSUFF: ${LIBC}
LIBC_PIC?=	${DESTDIR}/usr/lib/libc_pic.a
.NOSUFF: ${LIBC_PIC}
LIBCDK?=	${DESTDIR}/usr/lib/libcdk.a
.NOSUFF: ${LIBCDK}
LIBCOM_ERR?=	${DESTDIR}/usr/lib/libcom_err.a
.NOSUFF: ${LIBCOM_ERR}
LIBCOMPAT?=	${DESTDIR}/usr/lib/libcompat.a
.NOSUFF: ${LIBCOMPAT}
LIBCRYPT?=	${DESTDIR}/usr/lib/libcrypt.a
.NOSUFF: ${LIBCRYPT}
LIBCRYPTO?=	${DESTDIR}/usr/lib/libcrypto.a
.NOSUFF: ${LIBCRYPTO}
LIBCRYPTO_RC5?=	${DESTDIR}/usr/lib/libcrypto_rc5.a
.NOSUFF: ${LIBCRYPTO_RC5}
LIBCRYPTO_IDEA?=${DESTDIR}/usr/lib/libcrypto_idea.a
.NOSUFF: ${LIBCRYPTO_IDEA}
LIBCURSES?=	${DESTDIR}/usr/lib/libcurses.a
.NOSUFF: ${LIBCURSES}
LIBDBM?=	${DESTDIR}/usr/lib/libdbm.a
.NOSUFF: ${LIBDBM}
LIBDES?=	${DESTDIR}/usr/lib/libdes.a
.NOSUFF: ${LIBDES}
LIBEDIT?=	${DESTDIR}/usr/lib/libedit.a
.NOSUFF: ${LIBEDIT}
LIBFORM?=	${DESTDIR}/usr/lib/libform.a
.NOSUFF: ${LIBFORM}
LIBGCC?=	${DESTDIR}/usr/lib/libgcc.a
.NOSUFF: ${LIBGCC}
LIBGNUMALLOC?=	${DESTDIR}/usr/lib/libgnumalloc.a
.NOSUFF: ${LIBGNUMALLOC}
LIBGSSAPI?=	${DESTDIR}/usr/lib/libgssapi.a
.NOSUFF: ${LIBGSSAPI}
LIBHDB?=	${DESTDIR}/usr/lib/libhdb.a
.NOSUFF: ${LIBHDB}
LIBINTL?=	${DESTDIR}/usr/lib/libintl.a
.NOSUFF: ${LIBINTL}
LIBIPSEC?=	${DESTDIR}/usr/lib/libipsec.a
.NOSUFF: ${LIBIPSEC}
LIBKADM?=	${DESTDIR}/usr/lib/libkadm.a
.NOSUFF: ${LIBKADM}
LIBKADM5CLNT?=	${DESTDIR}/usr/lib/libkadm5clnt.a
.NOSUFF: ${LIBKADM5CLNT}
LIBKADM5SRV?=	${DESTDIR}/usr/lib/libkadm5srv.a
.NOSUFF: ${LIBKADM5SRV}
LIBKAFS?=	${DESTDIR}/usr/lib/libkafs.a
.NOSUFF: ${LIBKAFS}
LIBKDB?=	${DESTDIR}/usr/lib/libkdb.a
.NOSUFF: ${LIBKDB}
LIBKRB?=	${DESTDIR}/usr/lib/libkrb.a
.NOSUFF: ${LIBKRB}
LIBKRB5?=	${DESTDIR}/usr/lib/libkrb5.a
.NOSUFF: ${LIBKRB5}
LIBKSTREAM?=	${DESTDIR}/usr/lib/libkstream.a
.NOSUFF: ${LIBKSTREAM}
LIBKVM?=	${DESTDIR}/usr/lib/libkvm.a
.NOSUFF: ${LIBKVM}
LIBL?=		${DESTDIR}/usr/lib/libl.a
.NOSUFF: ${LIBL}
LIBM?=		${DESTDIR}/usr/lib/libm.a
.NOSUFF: ${LIBM}
LIBMENU?=	${DESTDIR}/usr/lib/libmenu.a
.NOSUFF: ${LIBMENU}
LIBOBJC?=	${DESTDIR}/usr/lib/libobjc.a
.NOSUFF: ${LIBOBJC}
LIBOSSAUDIO?=	${DESTDIR}/usr/lib/libossaudio.a
.NOSUFF: ${LIBOSSAUDIO}
LIBPCAP?=	${DESTDIR}/usr/lib/libpcap.a
.NOSUFF: ${LIBPCAP}
LIBPCI?=	${DESTDIR}/usr/lib/libpci.a
.NOSUFF: ${LIBPCI}
LIBPOSIX?=	${DESTDIR}/usr/lib/libposix.a
.NOSUFF: ${LIBPOSIX}
LIBRESOLV?=	${DESTDIR}/usr/lib/libresolv.a
.NOSUFF: ${LIBRESOLV}
LIBRMT?=	${DESTDIR}/usr/lib/librmt.a
.NOSUFF: ${LIBRMT}
LIBROKEN?=	${DESTDIR}/usr/lib/libroken.a
.NOSUFF: ${LIBROKEN}
LIBRPCSVC?=	${DESTDIR}/usr/lib/librpcsvc.a
.NOSUFF: ${LIBRPCSVC}
LIBSKEY?=	${DESTDIR}/usr/lib/libskey.a
.NOSUFF: ${LIBSKEY}
LIBSS?=		${DESTDIR}/usr/lib/libss.a
.NOSUFF: ${LIBSS}
LIBSSL?=	${DESTDIR}/usr/lib/libssl.a
.NOSUFF: ${LIBSSL}
LIBSL?=		${DESTDIR}/usr/lib/libsl.a
.NOSUFF: ${LIBSL}
LIBTERMCAP?=	${DESTDIR}/usr/lib/libtermcap.a
.NOSUFF: ${LIBTERMCAP}
LIBTELNET?=	${DESTDIR}/usr/lib/libtelnet.a
.NOSUFF: ${LIBTELNET}
LIBUSBHID?=	${DESTDIR}/usr/lib/libusbhid.a
.NOSUFF: ${LIBUSBHID}
LIBUTIL?=	${DESTDIR}/usr/lib/libutil.a
.NOSUFF: ${LIBUTIL}
LIBWRAP?=	${DESTDIR}/usr/lib/libwrap.a
.NOSUFF: ${LIBWRAP}
LIBY?=		${DESTDIR}/usr/lib/liby.a
.NOSUFF: ${LIBY}
LIBZ?=		${DESTDIR}/usr/lib/libz.a
.NOSUFF: ${LIBZ}

##### Build and install rules
.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c

.cc.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.cc
	@${CXX} ${CXXFLAGS} -c x.cc -o ${.TARGET}
	@rm -f x.cc

.C.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.C
	@${CXX} ${CXXFLAGS} -c x.C -o ${.TARGET}
	@rm -f x.C
.endif

.if defined(PROG_CXX)
PROG=	${PROG_CXX}
.endif

.if defined(PROG)
.if defined(PROG_CXX)
SRCS?=		${PROG}.cc
.else
SRCS?=		${PROG}.c
.endif

DPSRCS+=	${SRCS:M*.[ly]:C/\..$/.c/}
CLEANFILES+=	${DPSRCS} ${YHEADER:D${SRCS:M*.y:.y=.h}}

.if !empty(SRCS:N*.h:N*.sh:N*.fth)
OBJS+=		${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)
.NOPATH: ${OBJS} ${PROG} ${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}

_PROGLDOPTS=
.if ${SHLINKDIR} != "/usr/libexec"	# XXX: change or remove if ld.so moves
_PROGLDOPTS+=	-Wl,-dynamic-linker=${_SHLINKER}
.endif
.if ${SHLIBDIR} != ${LIBDIR}
_PROGLDOPTS+=	-Wl,-rpath-link,${DESTDIR}${SHLIBDIR}:${DESTDIR}/usr/lib \
		-Wl,-rpath,${SHLIBDIR}:/usr/lib \
		-L${DESTDIR}${SHLIBDIR}
.endif

.if defined(PROG_CXX)
_CCLINK=	${CXX}
_SUPCXX=	-lstdc++ -lm
.else
_CCLINK=	${CC}
.endif

.if defined(DESTDIR)

${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
.if !commands(${PROG})
	${_CCLINK} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} -nostdlib ${_PROGLDOPTS} ${LIBCRT0} ${LIBCRTBEGIN} ${OBJS} ${LDADD} -L${DESTDIR}/usr/lib ${_SUPCXX} -lgcc -lc -lgcc ${LIBCRTEND}
.endif

.else

${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
.if !commands(${PROG})
	${_CCLINK} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${_PROGLDOPTS} ${OBJS} ${LDADD}
.endif

.endif	# defined(DESTDIR)
.endif	# defined(OBJS) && !empty(OBJS)

.if !defined(MAN)
MAN=	${PROG}.1
.endif	# !defined(MAN)
.endif	# defined(PROG)

realall: ${PROG} ${SCRIPTS}

cleanprog: cleanobjs cleanextra
	rm -f a.out [Ee]rrs mklog core *.core ${PROG}

cleanobjs:
.if defined(OBJS) && !empty(OBJS)
	rm -f ${OBJS} ${LOBJS}
.endif

cleanextra:
.if defined(CLEANFILES) && !empty(CLEANFILES)
	rm -f ${CLEANFILES}
.endif

.if defined(SRCS) && !target(afterdepend)
afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.o \1.ln:/' \
	      < .depend > $$TMP; \
	    mv $$TMP .depend)
.endif

.if defined(PROG) && !target(proginstall)
PROGNAME?=${PROG}

proginstall:: ${DESTDIR}${BINDIR}/${PROGNAME}
.PRECIOUS: ${DESTDIR}${BINDIR}/${PROGNAME}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${BINDIR}/${PROGNAME}
.endif

__proginstall: .USE
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${STRIPFLAG} ${.ALLSRC} ${.TARGET}

.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${BINDIR}/${PROGNAME}: .MADE
.endif
${DESTDIR}${BINDIR}/${PROGNAME}: ${PROG} __proginstall
.endif

.if !target(proginstall)
proginstall::
.endif

.if defined(SCRIPTS) && !target(scriptsinstall)
SCRIPTSDIR?=${BINDIR}
SCRIPTSOWN?=${BINOWN}
SCRIPTSGRP?=${BINGRP}
SCRIPTSMODE?=${BINMODE}

scriptsinstall:: ${SCRIPTS:@S@${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}@}
.PRECIOUS: ${SCRIPTS:@S@${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}@}
.if !defined(UPDATE)
.PHONY: ${SCRIPTS:@S@${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}@}
.endif

__scriptinstall: .USE
	${INSTALL_FILE} \
	    -o ${SCRIPTSOWN_${.ALLSRC:T}:U${SCRIPTSOWN}} \
	    -g ${SCRIPTSGRP_${.ALLSRC:T}:U${SCRIPTSGRP}} \
	    -m ${SCRIPTSMODE_${.ALLSRC:T}:U${SCRIPTSMODE}} \
	    ${.ALLSRC} ${.TARGET}

.for S in ${SCRIPTS:O:u}
.if !defined(BUILD) && !make(all) && !make(${S})
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}: .MADE
.endif
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}: ${S} __scriptinstall
.endfor
.endif

.if !target(scriptsinstall)
scriptsinstall::
.endif

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:M-L*} ${LOBJS} ${LDADD}
.endif

##### Pull in related .mk logic
.include <bsd.man.mk>
.include <bsd.nls.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.sys.mk>

${TARGETS}:	# ensure existence
