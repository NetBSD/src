#	$NetBSD: bsd.prog.mk,v 1.127 2001/01/14 20:49:36 christos Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.depall.mk>
.MAIN:		all
.endif

.PHONY:		cleanprog proginstall scriptsinstall
realinstall:	proginstall scriptsinstall
clean cleandir distclean: cleanprog

CFLAGS+=	${COPTS}

# ELF platforms depend on crtbegin.o and crtend.o
.if ${OBJECT_FMT} == "ELF"
LIBCRTBEGIN?=	${DESTDIR}/usr/lib/crtbegin.o
LIBCRTEND?=	${DESTDIR}/usr/lib/crtend.o
.else
LIBCRTBEGIN?=
LIBCRTEND?=
.endif

LIBCRT0?=	${DESTDIR}/usr/lib/crt0.o

LIBBZ2?=	${DESTDIR}/usr/lib/libbz2.a
LIBC?=		${DESTDIR}/usr/lib/libc.a
LIBC_PIC?=	${DESTDIR}/usr/lib/libc_pic.a
LIBCDK?=	${DESTDIR}/usr/lib/libcdk.a
LIBCOM_ERR?=	${DESTDIR}/usr/lib/libcom_err.a
LIBCOMPAT?=	${DESTDIR}/usr/lib/libcompat.a
LIBCRYPT?=	${DESTDIR}/usr/lib/libcrypt.a
LIBCRYPTO?=	${DESTDIR}/usr/lib/libcrypto.a
LIBCRYPTO_RC5?=	${DESTDIR}/usr/lib/libcrypto_rc5.a
LIBCRYPTO_IDEA?=${DESTDIR}/usr/lib/libcrypto_idea.a
LIBCURSES?=	${DESTDIR}/usr/lib/libcurses.a
LIBDBM?=	${DESTDIR}/usr/lib/libdbm.a
LIBDES?=	${DESTDIR}/usr/lib/libdes.a
LIBEDIT?=	${DESTDIR}/usr/lib/libedit.a
LIBFORM?=	${DESTDIR}/usr/lib/libform.a
LIBGCC?=	${DESTDIR}/usr/lib/libgcc.a
LIBGNUMALLOC?=	${DESTDIR}/usr/lib/libgnumalloc.a
LIBGSSAPI?=	${DESTDIR}/usr/lib/libgssapi.a
LIBHDB?=	${DESTDIR}/usr/lib/libhdb.a
LIBINTL?=	${DESTDIR}/usr/lib/libintl.a
LIBIPSEC?=	${DESTDIR}/usr/lib/libipsec.a
LIBKADM?=	${DESTDIR}/usr/lib/libkadm.a
LIBKADM5CLNT?=	${DESTDIR}/usr/lib/libkadm5clnt.a
LIBKADM5SRV?=	${DESTDIR}/usr/lib/libkadm5srv.a
LIBKAFS?=	${DESTDIR}/usr/lib/libkafs.a
LIBKDB?=	${DESTDIR}/usr/lib/libkdb.a
LIBKRB?=	${DESTDIR}/usr/lib/libkrb.a
LIBKRB5?=	${DESTDIR}/usr/lib/libkrb5.a
LIBKSTREAM?=	${DESTDIR}/usr/lib/libkstream.a
LIBKVM?=	${DESTDIR}/usr/lib/libkvm.a
LIBL?=		${DESTDIR}/usr/lib/libl.a
LIBM?=		${DESTDIR}/usr/lib/libm.a
LIBMENU?=	${DESTDIR}/usr/lib/libmenu.a
LIBOBJC?=	${DESTDIR}/usr/lib/libobjc.a
LIBOSSAUDIO?=	${DESTDIR}/usr/lib/libossaudio.a
LIBPCAP?=	${DESTDIR}/usr/lib/libpcap.a
LIBPOSIX?=	${DESTDIR}/usr/lib/libposix.a
LIBRESOLV?=	${DESTDIR}/usr/lib/libresolv.a
LIBRMT?=	${DESTDIR}/usr/lib/librmt.a
LIBROKEN?=	${DESTDIR}/usr/lib/libroken.a
LIBRPCSVC?=	${DESTDIR}/usr/lib/librpcsvc.a
LIBSKEY?=	${DESTDIR}/usr/lib/libskey.a
LIBSS?=		${DESTDIR}/usr/lib/libss.a
LIBSSL?=	${DESTDIR}/usr/lib/liblss.a
LIBSL?=		${DESTDIR}/usr/lib/libsl.a
LIBTERMCAP?=	${DESTDIR}/usr/lib/libtermcap.a
LIBTELNET?=	${DESTDIR}/usr/lib/libtelnet.a
LIBUSB?=	${DESTDIR}/usr/lib/libusb.a
LIBUTIL?=	${DESTDIR}/usr/lib/libutil.a
LIBWRAP?=	${DESTDIR}/usr/lib/libwrap.a
LIBY?=		${DESTDIR}/usr/lib/liby.a
LIBZ?=		${DESTDIR}/usr/lib/libz.a

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


.if defined(PROG)
SRCS?=		${PROG}.c

DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${DPSRCS}
.if defined(YHEADER)
CLEANFILES+=	${SRCS:M*.y:.y=.h}
.endif

.if !empty(SRCS:N*.h:N*.sh:N*.fth)
OBJS+=		${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)
.NOPATH: ${OBJS}
.if defined(DESTDIR)

${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
.if !commands(${PROG})
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} -nostdlib -Wl,-rpath-link,${DESTDIR}/usr/lib ${LIBCRT0} ${LIBCRTBEGIN} ${OBJS} ${LDADD} -L${DESTDIR}/usr/lib -lgcc -lc -lgcc ${LIBCRTEND}
.endif

.else

${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
.if !commands(${PROG})
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${OBJS} ${LDADD}
.endif

.endif	# defined(DESTDIR)
.endif	# defined(OBJS) && !empty(OBJS)

.if !defined(MAN)
MAN=	${PROG}.1
.endif	# !defined(MAN)
.endif	# defined(PROG)

realall: ${PROG} ${SCRIPTS}

cleanprog:
	rm -f a.out [Ee]rrs mklog core *.core \
	    ${PROG} ${OBJS} ${LOBJS} ${CLEANFILES}

.if defined(SRCS)
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
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	    -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.ALLSRC} ${.TARGET}

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
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} \
	    -o ${SCRIPTSOWN_${.ALLSRC:T}:U${SCRIPTSOWN}} \
	    -g ${SCRIPTSGRP_${.ALLSRC:T}:U${SCRIPTSGRP}} \
	    -m ${SCRIPTSMODE_${.ALLSRC:T}:U${SCRIPTSMODE}} \
	    ${.ALLSRC} ${.TARGET}

.for S in ${SCRIPTS}
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

.include <bsd.man.mk>
.include <bsd.nls.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.sys.mk>

# Make sure all of the standard targets are defined, even if they do nothing.
regress:
