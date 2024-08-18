#	$NetBSD: Makefile,v 1.37 2024/08/18 20:50:00 christos Exp $

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
is_ip_address.c \
lib_strbuf.c \
libssl_compat.c \
machines.c \
mktime.c \
modetoa.c \
mstolfp.c \
msyslog.c \
netof.c \
ntp_calendar.c \
ntp_calgps.c \
ntp_crypto_rnd.c \
ntp_intres.c \
ntp_libopts.c \
ntp_lineedit.c \
ntp_random.c \
ntp_realpath.c \
ntp_rfc2553.c \
ntp_worker.c \
numtoa.c \
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
timespecops.c \
timetoa.c \
timexsup.c \
uglydate.c \
vint64ops.c \
work_fork.c \
work_thread.c \
xsbprintf.c \
ymd2yd.c

CPPFLAGS+= -I${IDIST}/sntp/libopts

# For MKREPRO, avoid using __DATE__ and __TIME__.
.if ${MKREPRO:Uno} == "yes"
MKREPRO_CPPFLAGS.ntp_calendar.c:= \
	-DMKREPRO_DATE=\"${%b %e %Y:L:${:Ugmtime=${MKREPRO_TIMESTAMP}}:Q}\" \
	-DMKREPRO_TIME=\"${%T:L:${:Ugmtime=${MKREPRO_TIMESTAMP}}:Q}\"
CPPFLAGS.ntp_calendar.c += ${MKREPRO_CPPFLAGS.ntp_calendar.c}
.endif

COPTS.timetoa.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.socktoa.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-restrict :}
COPTS.socktohost.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-restrict :}

COPTS.a_md5encrypt.c+= -Wno-error=deprecated-declarations

.include <bsd.lib.mk>
