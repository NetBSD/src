#	$NetBSD: Makefile,v 1.3 1998/04/12 13:59:28 veego Exp $

LIB=	f2c

# libF77:
FSRCS=	F77_aloc.c Version.c main.c s_rnge.c abort_.c getarg_.c iargc_.c \
	getenv_.c signal_.c s_stop.c s_paus.c system_.c cabs.c derf_.c \
	derfc_.c erf_.c erfc_.c sig_die.c exit_.c pow_ci.c pow_dd.c pow_di.c \
	pow_hh.c pow_ii.c  pow_ri.c pow_zi.c pow_zz.c pow_qq.c c_abs.c \
	c_cos.c c_div.c c_exp.c c_log.c c_sin.c c_sqrt.c z_abs.c z_cos.c \
	z_div.c z_exp.c z_log.c z_sin.c z_sqrt.c r_abs.c r_acos.c r_asin.c \
	r_atan.c r_atn2.c r_cnjg.c r_cos.c r_cosh.c r_dim.c r_exp.c r_imag.c \
	r_int.c r_lg10.c r_log.c r_mod.c r_nint.c r_sign.c r_sin.c r_sinh.c \
	r_sqrt.c r_tan.c r_tanh.c d_abs.c d_acos.c d_asin.c d_atan.c d_atn2.c \
	d_cnjg.c d_cos.c d_cosh.c d_dim.c d_exp.c d_imag.c d_int.c d_lg10.c \
	d_log.c d_mod.c d_nint.c d_prod.c d_sign.c d_sin.c d_sinh.c d_sqrt.c \
	d_tan.c d_tanh.c i_abs.c i_dim.c i_dnnt.c i_indx.c i_len.c i_mod.c \
	i_nint.c i_sign.c h_abs.c h_dim.c h_dnnt.c h_indx.c h_len.c h_mod.c \
	h_nint.c h_sign.c l_ge.c l_gt.c l_le.c l_lt.c hl_ge.c hl_gt.c hl_le.c \
	hl_lt.c ef1asc_.c ef1cmc_.c s_cat.c s_cmp.c s_copy.c lbitbits.c \
	lbitshft.c qbitbits.c qbitshft.c

# libI77:
ISRCS=	Version.c backspace.c close.c dfe.c dolio.c due.c endfile.c err.c \
	fmt.c fmtlib.c iio.c ilnw.c inquire.c lread.c lwrite.c open.c \
	rdfmt.c rewind.c rsfe.c rsli.c rsne.c sfe.c sue.c typesize.c uio.c \
	util.c wref.c wrtfmt.c wsfe.c wsle.c wsne.c xwsne.c ftell_.c

# libU77:
USRCS=	Version.c access_.c alarm_.c bes.c chdir_.c chmod_.c ctime_.c date_.c \
	dbes.c dtime_.c etime_.c fdate_.c fgetc_.c flush1_.c fnum_.c fputc_.c \
	fstat_.c gerror_.c getcwd_.c getgid_.c getlog_.c getpid_.c getuid_.c \
	gmtime_.c hostnm_.c idate_.c ierrno_.c irand_.c isatty_.c itime_.c \
	kill_.c link_.c lnblnk_.c lstat_.c ltime_.c mclock_.c perror_.c \
	rand_.c rename_.c secnds_.c second_.c sleep_.c srand_.c stat_.c \
	symlnk_.c sys_clock_.c time_.c ttynam_.c umask_.c unlink_.c \
	vxtidate_.c vxttime_.c

SRCS=	${FSRCS:S@^@libF77/@} ${ISRCS:S@^@libI77/@} ${USRCS:S@^@libU77/@} \
	f2cext.c
RT=	${.CURDIR}/../../dist/gcc/f/runtime
NOLINT=

CPPFLAGS+=	-I. -I${.CURDIR} -I${RT}/libF77 -I${RT}/libI77 -I${RT}/libU77 \
		-DNETBSD_NATIVE -DHAVE_CONFIG_H=1 -DMISSING_FILE_ELEMS=1 \
		-DRETSIGTYPE=void -DIEEE_drem=1 -DNON_ANSI_RW_MODES=1 \
		-DNO_EOF_CHAR_CHECK=1 -DSkip_f2c_Undefs=1 -DPad_UDread=1 \
		-DWANT_LEAD_0=1

.PATH:	${RT}

${SRCS}: f2c.h
f2c.h: f2c.h.in
	sed -e 's/@F2C_INTEGER@/int/g' -e's/@F2C_LONGINT@/long/g' < ${.ALLSRC} > f2c.h
CLEANFILES+=	f2c.h

${SRCS}: dirs
dirs:
	mkdir libF77 libI77 libU77 || true
	@touch dirs

.include <bsd.lib.mk>

clean cleandir: __cleanlibdir
.PHONY: __cleanlibdir
__cleanlibdir:
	rm -rf dirs libF77 libI77 libU77
