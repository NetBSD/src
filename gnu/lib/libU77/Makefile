#	$NetBSD: Makefile,v 1.1 1998/03/30 03:00:20 mrg Exp $

LIB=	U77
SRCS=	Version.c access_.c alarm_.c bes.c chdir_.c chmod_.c ctime_.c date_.c \
	dbes.c dtime_.c etime_.c fdate_.c fgetc_.c flush1_.c fnum_.c fputc_.c \
	fstat_.c gerror_.c getcwd_.c getgid_.c getlog_.c getpid_.c getuid_.c \
	gmtime_.c hostnm_.c idate_.c ierrno_.c irand_.c isatty_.c itime_.c \
	kill_.c link_.c lnblnk_.c lstat_.c ltime_.c mclock_.c perror_.c \
	rand_.c rename_.c secnds_.c second_.c sleep_.c srand_.c stat_.c \
	symlnk_.c sys_clock_.c time_.c ttynam_.c umask_.c unlink_.c \
	vxtidate_.c vxttime_.c

CPPFLAGS+=	-I$(DIST)/f/runtime/libI77

.include <bsd.lib.mk>
.include "../Makefile.egcsflib"
