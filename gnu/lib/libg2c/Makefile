#	$NetBSD: Makefile,v 1.8 2001/01/04 09:22:06 itojun Exp $

.include <bsd.own.mk>

LIB=		g2c

F77SRCS=	F77_aloc.c Version.c main.c s_rnge.c abort_.c getarg_.c \
		iargc_.c getenv_.c signal_.c s_stop.c s_paus.c system_.c \
		cabs.c derf_.c derfc_.c erf_.c erfc_.c sig_die.c exit_.c \
		setarg.c setsig.c pow_ci.c pow_dd.c pow_di.c pow_hh.c \
		pow_ii.c pow_ri.c pow_zi.c pow_zz.c pow_qq.c c_abs.c \
		c_cos.c c_div.c c_exp.c c_log.c c_sin.c c_sqrt.c z_abs.c \
		z_cos.c z_div.c z_exp.c z_log.c z_sin.c z_sqrt.c r_abs.c \
		r_acos.c r_asin.c r_atan.c r_atn2.c r_cnjg.c r_cos.c \
		r_cosh.c r_dim.c r_exp.c r_imag.c r_int.c r_lg10.c r_log.c \
		r_mod.c r_nint.c r_sign.c r_sin.c r_sinh.c r_sqrt.c \
		r_tan.c r_tanh.c d_abs.c d_acos.c d_asin.c d_atan.c \
		d_atn2.c d_cnjg.c d_cos.c d_cosh.c d_dim.c d_exp.c \
		d_imag.c d_int.c d_lg10.c d_log.c d_mod.c d_nint.c \
		d_prod.c d_sign.c d_sin.c d_sinh.c d_sqrt.c d_tan.c \
		d_tanh.c i_abs.c i_dim.c i_dnnt.c i_indx.c i_len.c i_mod.c \
		i_nint.c i_sign.c h_abs.c h_dim.c h_dnnt.c h_indx.c \
		h_len.c h_mod.c h_nint.c h_sign.c l_ge.c l_gt.c l_le.c \
		l_lt.c hl_ge.c hl_gt.c hl_le.c hl_lt.c ef1asc_.c ef1cmc_.c \
		s_cat.c s_cmp.c s_copy.c lbitbits.c lbitshft.c qbitbits.c \
		qbitshft.c

I77SRCS=	Version.c backspace.c close.c dfe.c dolio.c due.c \
		endfile.c err.c fmt.c fmtlib.c iio.c ilnw.c inquire.c \
		lread.c lwrite.c open.c rdfmt.c rewind.c rsfe.c rsli.c \
		rsne.c sfe.c sue.c typesize.c uio.c util.c wref.c wrtfmt.c \
		wsfe.c wsle.c wsne.c xwsne.c ftell_.c

U77SRCS=	Version.c gerror_.c perror_.c ierrno_.c itime_.c time_.c \
		unlink_.c fnum_.c getpid_.c getuid_.c getgid_.c kill_.c \
		rand_.c srand_.c irand_.c sleep_.c idate_.c ctime_.c \
		etime_.c dtime_.c isatty_.c ltime_.c fstat_.c stat_.c \
		lstat_.c access_.c link_.c getlog_.c ttynam_.c getcwd_.c \
		symlnk_.c vxttime_.c vxtidate_.c gmtime_.c fdate_.c \
		secnds_.c bes.c dbes.c chdir_.c chmod_.c lnblnk_.c \
		hostnm_.c rename_.c fgetc_.c fputc_.c umask_.c \
		sys_clock_.c date_.c second_.c flush1_.c mclock_.c \
		alarm_.c datetime_.c

E77OBJS=	abort.o derf.o derfc.o ef1asc.o ef1cmc.o erf.o erfc.o \
		exit.o getarg.o getenv.o iargc.o signal.o system.o flush.o \
		ftell.o fseek.o access.o besj0.o besj1.o besjn.o besy0.o \
		besy1.o besyn.o chdir.o chmod.o ctime.o date.o dbesj0.o \
		dbesj1.o dbesjn.o dbesy0.o dbesy1.o dbesyn.o dtime.o \
		etime.o fdate.o fgetc.o fget.o flush1.o fnum.o fputc.o \
		fput.o fstat.o gerror.o getcwd.o getgid.o getlog.o \
		getpid.o getuid.o gmtime.o hostnm.o idate.o ierrno.o \
		irand.o isatty.o itime.o kill.o link.o lnblnk.o lstat.o \
		ltime.o mclock.o perror.o rand.o rename.o secnds.o \
		second.o sleep.o srand.o stat.o symlnk.o time.o ttynam.o \
		umask.o unlink.o vxtidt.o vxttim.o alarm.o

LIBF77SRCS=	${F77SRCS:.c=_F.c} ${I77SRCS:.c=_I.c} ${U77SRCS:.c=_U.c}

SRCS+=		${LIBF77SRCS}
OBJS+=		${E77OBJS}
.if ${MKPIC} != "no"
SOBJS+=		${E77OBJS:.o=.so}
.endif
.if ${MKPROFILE} != "no"
POBJS+=		${E77OBJS:.o=.po}
.endif
CLEANFILES+=	${LIBF77SRCS}
DPSRCS+=	g2c.h

MKLINT=		no

.if (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb")
ARCHSUBDIR= mips
.elif (${MACHINE_ARCH} == "sh3el") || (${MACHINE_ARCH} == "sh3eb")
ARCHSUBDIR= sh3
.else
ARCHSUBDIR= ${MACHINE_ARCH}
.endif

CPPFLAGS+=	-I. -I${.CURDIR} -I${ARCH} -I${ARCH}/${ARCHSUBDIR} \
		-I${DIST}/libf2c -I${DIST}/libf2c/libF77 \
		-I${DIST}/libf2c/libI77 -I${DIST}/libf2c/libU77 \
		-I${DIST}/gcc/config -I${DIST}/gcc/f -I${DIST}/gcc \
		-DNETBSD_NATIVE -DHAVE_CONFIG_H=1 -DSTDC_HEADERS=1 \
		-DRETSIGTYPE=void -DSkip_f2c_Undefs=1 \
		-DHAVE_TEMPNAM=1 -DNO_EOF_CHAR_CHECK=1
.if (${MACHINE_ARCH} != "vax")
CPPFLAGS+=	-DIEEE_drem=1
.endif

CLEANFILES+=	g2c.h g2ctypes.sed

# XXX may need a better way to do this; can be overridden with "cp"
MAKE_SYMLINK?=	ln -sf

DIST=		${.CURDIR}/../../dist
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

.PATH:${DIST}/libf2c ${DIST}/gcc/f

g2ctypes.sed: g2ctypes.c
	${CC} ${CPPFLAGS} -E ${.CURDIR}/g2ctypes.c | grep 'F2C_' >g2ctypes.sed

g2c.h: g2c.hin g2ctypes.sed
	sed -f g2ctypes.sed <${.ALLSRC:M*.hin} >$@

.for file in ${F77SRCS}
${file:.c=_F.c}: ${DIST}/libf2c/libF77/${file}
	${MAKE_SYMLINK} $> $@
.endfor

.for file in ${I77SRCS}
${file:.c=_I.c}: ${DIST}/libf2c/libI77/${file}
	${MAKE_SYMLINK} $> $@
.endfor

.for file in ${U77SRCS}
${file:.c=_U.c}: ${DIST}/libf2c/libU77/${file}
	${MAKE_SYMLINK} $> $@
.endfor

${E77OBJS}: f2cext.c
	${COMPILE.c} -DL$* -o $@ ${.ALLSRC:M*.c}

.if ${MKPIC} != "no"
${E77OBJS:.o=.so}: f2cext.c
	${COMPILE.c} ${CPICFLAGS} -DL$* -o $@ ${.ALLSRC:M*.c}
.endif

.if ${MKPROFILE} != "no"
${E77OBJS:.o=.po}: f2cext.c
	${COMPILE.c} -pg -DL$* -o $@ ${.ALLSRC:M*.c}
.endif

.include <bsd.lib.mk>

${OBJS} ${SOBJS} ${POBJS}: g2c.h
