#	$NetBSD: Makefile,v 1.260 2012/01/04 16:25:14 yamt Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton acpitools altq apm apmd arp \
	bad144 bootp btattach btconfig btdevctl bthcid btpand catman \
	chroot cnwctl cpuctl crash \
	dev_mkdb dhcp diskpart dumpfs dumplfs \
	edquota eeprom envstat eshconfig etcupdate extattrctl \
	flashctl fssconfig fusermount fwctl \
	gpioctl grfconfig gspa \
	hdaudioctl \
	i2cscan ifwatchd inetd installboot iopctl iostat ipwctl irdaattach \
	isdn isibootd iteconfig iwictl \
	kgmon \
	lastlogin ldpd link lmcconfig lockstat lpr \
	mailwrapper makefs map-mbone mdconfig memswitch mlxctl mmcformat \
	mopd mountd moused mrinfo mrouted mscdlabel mtrace mtree \
	ndbootd ndiscvt netgroup_mkdb nfsd \
	ofctl \
	paxctl pcictl perfused pppd psrset pstat pwd_mkdb postinstall \
	powerd puffs \
	quot quotacheck quotactl quotaon \
	rarpd rbootd rdate repquota rmt rpc.bootparamd rpc.lockd \
	rpc.pcnfsd rpc.statd rpcbind rwhod \
	sa screenblank sdpd services_mkdb sesd schedctl sliplogin spray \
	srtconfig sti sunlabel sup syslogd \
	tadpolectl tcpdchk tcpdmatch tcpdrop timed tpctl tprof traceroute trpt \
	unlink usbdevs user \
	videomode vipw veriexecgen vnconfig \
	wake wiconfig wlanctl wsconscfg wsfontload wsmoused wsmuxctl \
	zdump zic

.if (${MKYP} != "no")
SUBDIR+= rpc.yppasswdd ypbind yppoll ypserv ypset
.endif

.if ${TOOLCHAIN_MISSING} != "no"
SUBDIR+= mdsetimage
.endif

.if (${MKINET6} != "no")
SUBDIR+=faithd ifmcstat ndp rip6query rtsold
.endif
.if (${USE_INET6} != "no")
SUBDIR+=mld6query route6d rtadvd traceroute6 
.endif

# ATM PVC
SUBDIR+=pvcsif pvctxctl

.if (${MKCRYPTO} != "no")
SUBDIR+= racoon racoonctl
.endif

# NPF
.if (${MKNPF} != "no")
SUBDIR+=npf
.endif

# IP Filter
.if (${MKIPFILTER} != "no")
SUBDIR+=ipf
.endif

# PF
.if (${MKPF} != "no")
SUBDIR+=pf
.endif

.include <bsd.subdir.mk>
