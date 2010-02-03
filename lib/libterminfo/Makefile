#	$NetBSD: Makefile,v 1.1 2010/02/03 15:16:32 roy Exp $

USE_SHLIBDIR=	yes

LIB=		terminfo
WARNS=		4

SRCS=		term.c ti.c setupterm.c curterm.c tparm.c tputs.c
SRCS+=		hash.c
INCS=		term.h
INCSDIR=	/usr/include

LIBTIDIR?=	${.CURDIR}
CPPFLAGS+=	-I${LIBTIDIR}

# Generate our string and hash tables
hash.c: genhash term.h
		@echo "Generating terminfo hash"
		cd ${LIBTIDIR}; ./genhash >${.OBJDIR}/$@

# Update our man page with terminfo long names, short names and termcaps
terminfo.5: genman terminfo.5.in term.h termcap.c
		@echo "Generating terminfo man pages"
		cd ${LIBTIDIR}; ./genman >${.OBJDIR}/$@

MAN=		terminfo.3 terminfo.5
CLEANFILES+=	terminfo.5 hash.c
MLINKS=		terminfo.3 setupterm.3 \
		terminfo.3 set_curterm.3 terminfo.3 del_curterm.3 \
		terminfo.3 tigetnum.3 terminfo.3 tigetflag.3 \
		terminfo.3 tigetstr.3 terminfo.3 tparm.3 terminfo.3 tputs.3 \
		terminfo.3 putp.3 \
		terminfo.3 ti_setupterm.3 terminfo.3 ti_getflag.3 \
		terminfo.3 ti_getnum.3 terminfo.3 ti_getstr.3 \
		terminfo.3 t_parm.3 terminfo.3 t_vparm.3 \
		terminfo.3 ti_puts.3 terminfo.3 ti_putp.3

# Build in termcap emulation
SRCS+=		termcap.c
INCS+=		termcap.h
CPPFLAGS+=	-I${.OBJDIR}
MAN+=		termcap.3
CLEANFILES+=	_termcap.c termcap_hash.c
MLINKS+=	termcap.3 tgetent.3 termcap.3 tgetflag.3 termcap.3 tgetnum.3 \
		termcap.3 tgetstr.3 termcap.3 tgoto.3

# Generate our string and hash tables
termcap_hash.c:	genthash termcap_map.c
		@echo "Generating termcap hash"
		cd ${LIBTIDIR}; ./genthash >${.OBJDIR}/$@

# Depend on our hash table
termcap.c:	termcap_hash.c

.include <bsd.own.mk>
.include <bsd.shlib.mk>

.if ${MKLINKLIB} != "no"
SYMLINKS+=	libterminfo.a ${LIBDIR}/libtermcap.a
SYMLINKS+=	libterminfo.a ${LIBDIR}/libtermlib.a
.endif

.if ${MKPROFILE} != "no"
SYMLINKS+=	libterminfo_p.a ${LIBDIR}/libtermcap_p.a
SYMLINKS+=	libterminfo_p.a ${LIBDIR}/libtermlib_p.a
.endif

.if ${MKPIC} != "no"

.if ${MKPICINSTALL} != "no"
SYMLINKS+=	libterminfo_pic.a ${LIBDIR}/libtermcap_pic.a
SYMLINKS+=	libterminfo_pic.a ${LIBDIR}/libtermlib_pic.a
.endif

.if exists(${.CURDIR}/shlib_version)
SYMLINKS+=	libterminfo.so.${SHLIB_FULLVERSION} \
		${_LIBSODIR}/libtermcap.so.0.6
SYMLINKS+=	libterminfo.so.${SHLIB_FULLVERSION} \
		${_LIBSODIR}/libtermlib.so.0.6
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libterminfo.so.${SHLIB_FULLVERSION} \
		${LIBDIR}/libtermcap.so.0.6
SYMLINKS+=	libterminfo.so.${SHLIB_FULLVERSION} \
		${LIBDIR}/libtermlib.so.0.6
.endif

.if (${OBJECT_FMT} == "ELF")
SYMLINKS+=	libterminfo.so.${SHLIB_MAJOR} \
		${_LIBSODIR}/libtermcap.so.0
SYMLINKS+=	libterminfo.so ${_LIBSODIR}/libtermcap.so
SYMLINKS+=	libterminfo.so.${SHLIB_MAJOR} \
		${_LIBSODIR}/libtermlib.so.0
SYMLINKS+=	libterminfo.so ${_LIBSODIR}/libtermlib.so
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libterminfo.so.${SHLIB_MAJOR} \
	${LIBDIR}/libtermcap.so.0
SYMLINKS+=	libterminfo.so ${LIBDIR}/libtermcap.so
SYMLINKS+=	libterminfo.so.${SHLIB_MAJOR} \
		${LIBDIR}/libtermlib.so.0
SYMLINKS+=	libterminfo.so ${LIBDIR}/libtermlib.so
.endif
.endif
.endif	# exists shlib_version

.endif	# ${MKPIC} != "no"

.include <bsd.lib.mk>
