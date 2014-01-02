#	$NetBSD: Makefile,v 1.7 2014/01/02 21:38:38 joerg Exp $

LIBISPRIVATE=yes

LIB=ntp

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/libntp 

.PATH.c:	${DIST}

SRCS= \
a_md5encrypt.c \
adjtime.c \
atoint.c \
atolfp.c \
atouint.c \
audio.c \
authkeys.c \
authreadkeys.c \
authusekey.c \
bsd_strerror.c \
buftvtots.c \
caljulian.c \
caltontp.c \
calyearstart.c \
clocktime.c \
clocktypes.c \
decodenetnum.c \
dofptoa.c \
dolfptoa.c \
emalloc.c \
findconfig.c \
getopt.c \
hextoint.c \
hextolfp.c \
humandate.c \
icom.c \
iosignal.c \
lib_strbuf.c \
machines.c \
mktime.c \
modetoa.c \
mstolfp.c \
msyslog.c \
netof.c \
ntp_calendar.c \
ntp_intres.c \
ntp_libopts.c \
ntp_lineedit.c \
ntp_random.c \
ntp_rfc2553.c \
ntp_worker.c \
numtoa.c \
numtohost.c \
octtoint.c \
prettydate.c \
recvbuff.c \
refnumtoa.c \
snprintf.c \
socket.c \
socktoa.c \
socktohost.c \
ssl_init.c \
statestr.c \
strdup.c \
syssignal.c \
systime.c \
timetoa.c \
uglydate.c \
work_fork.c \
work_thread.c \
ymd2yd.c

CPPFLAGS+= -I${IDIST}/sntp/libopts

COPTS.msyslog.c+=	-Wno-error=format-nonliteral

.include <bsd.lib.mk>
