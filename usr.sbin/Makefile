#	$NetBSD: Makefile,v 1.70 1997/01/02 00:34:53 pk Exp $
#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91

# not yet done: catman

SUBDIR=	ac accton amd arp bootpd bootpgw bootpef bootptest \
	chown chroot config cron dev_mkdb dhcpd \
	diskpart edquota gettable gspa htable inetd iostat kgmon \
	kvm_mkdb lpr map-mbone mrinfo mrouted mtrace mtree named \
	netgroup_mkdb portmap pppd pstat pwd_mkdb quot quotaon \
	rarpd rbootd rdate mdconfig mdsetimage repquota rmt \
	rpc.bootparamd rpc.pcnfsd rpc.yppasswdd rwhod \
	sa sendmail sliplogin \
	slstats spray sysctl \
	syslogd tcpdump timed traceroute trpt trsp update \
	vipw vnconfig ypbind yppoll ypserv ypset zdump zic

.if   make(clean) || make(cleandir)
SUBDIR+=apm apmd bad144 config.old dbsym eeprom grfconfig grfinfo hilinfo \
	iteconfig screenblank videomode
.elif ${MACHINE} == "amiga"
SUBDIR+=grfconfig iteconfig screenblank videomode
.elif ${MACHINE} == "atari"
SUBDIR+=iteconfig
.elif ${MACHINE} == "hp300"
SUBDIR+=config.old grfinfo hilinfo
.elif ${MACHINE} == "i386"
SUBDIR+=apm apmd bad144
.elif ${MACHINE} == "pmax"
SUBDIR+=screenblank
.elif ${MACHINE} == "sun3"
SUBDIR+=dbsym eeprom screenblank
.elif ${MACHINE} == "sparc"
SUBDIR+=eeprom screenblank
.endif

.include <bsd.subdir.mk>
