#	$NetBSD: bsd.hostprog.mk,v 1.12 2001/09/13 23:23:26 thorpej Exp $
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

.PHONY:		cleanprog 
clean:		cleanprog

CFLAGS+=	${COPTS}

LIBBZ2?=	/usr/lib/libbz2.a
LIBC?=		/usr/lib/libc.a
LIBC_PIC?=	/usr/lib/libc_pic.a
LIBCDK?=	/usr/lib/libcdk.a
LIBCOMPAT?=	/usr/lib/libcompat.a
LIBCRYPT?=	/usr/lib/libcrypt.a
LIBCURSES?=	/usr/lib/libcurses.a
LIBDBM?=	/usr/lib/libdbm.a
LIBDES?=	/usr/lib/libdes.a
LIBEDIT?=	/usr/lib/libedit.a
LIBFORM?=	/usr/lib/libform.a
LIBGCC?=	/usr/lib/libgcc.a
LIBGNUMALLOC?=	/usr/lib/libgnumalloc.a
LIBINTL?=	/usr/lib/libintl.a
LIBIPSEC?=	/usr/lib/libipsec.a
LIBKDB?=	/usr/lib/libkdb.a
LIBKRB?=	/usr/lib/libkrb.a
LIBKVM?=	/usr/lib/libkvm.a
LIBL?=		/usr/lib/libl.a
LIBM?=		/usr/lib/libm.a
LIBMENU?=	/usr/lib/libmenu.a
LIBMP?=		/usr/lib/libmp.a
LIBNTP?=	/usr/lib/libntp.a
LIBOBJC?=	/usr/lib/libobjc.a
LIBPC?=		/usr/lib/libpc.a
LIBPCAP?=	/usr/lib/libpcap.a
LIBPCI?=	/usr/lib/libpci.a
LIBPLOT?=	/usr/lib/libplot.a
LIBPOSIX?=	/usr/lib/libposix.a
LIBRESOLV?=	/usr/lib/libresolv.a
LIBRPCSVC?=	/usr/lib/librpcsvc.a
LIBSKEY?=	/usr/lib/libskey.a
LIBTERMCAP?=	/usr/lib/libtermcap.a
LIBTELNET?=	/usr/lib/libtelnet.a
LIBUTIL?=	/usr/lib/libutil.a
LIBWRAP?=	/usr/lib/libwrap.a
LIBY?=		/usr/lib/liby.a
LIBZ?=		/usr/lib/libz.a

.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.lo:
	${HOST_CC} -E ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${HOST_CC} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c

.cc.lo:
	${HOST_CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.cc
	@${HOST_CXX} ${CXXFLAGS} -c x.cc -o ${.TARGET}
	@rm -f x.cc

.C.lo:
	${HOST_CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.C
	@${HOST_CXX} ${CXXFLAGS} -c x.C -o ${.TARGET}
	@rm -f x.C
.endif


.if defined(HOSTPROG)
SRCS?=		${HOSTPROG}.c

DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${DPSRCS}
.if defined(YHEADER)
CLEANFILES+=	${SRCS:M*.y:.y=.h}
.endif

.if !empty(SRCS:N*.h:N*.sh)
OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.lo/g}
LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)
.NOPATH: ${OBJS}

${HOSTPROG}: ${DPSRCS} ${OBJS} ${LIBC} ${DPADD}
	${HOST_LINK.c} ${HOST_LDSTATIC} -o ${.TARGET} ${OBJS} ${LDADD}

.endif	# defined(OBJS) && !empty(OBJS)

.if !defined(MAN)
MAN=	${HOSTPROG}.1
.endif	# !defined(MAN)
.endif	# defined(HOSTPROG)

realall: ${HOSTPROG}

cleanprog:
	rm -f a.out [Ee]rrs mklog core *.core \
	    ${HOSTPROG} ${OBJS} ${LOBJS} ${CLEANFILES}

beforedepend:
CPPFLAGS=	${HOST_CPPFLAGS}

.if defined(SRCS)
afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.lo \1.ln:/' \
	      < .depend > $$TMP; \
	    mv $$TMP .depend)
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
