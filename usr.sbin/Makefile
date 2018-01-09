#	$NetBSD: Makefile,v 1.276 2018/01/09 03:31:13 christos Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton acpitools altq apm apmd arp autofs \
	bad144 bootp bta2dpd btattach btconfig btdevctl bthcid btpand catman \
	chroot cnwctl cpuctl crash \
	dev_mkdb diskpart dumpfs dumplfs \
	edquota eeprom envstat eshconfig etcupdate extattrctl \
	flashctl fssconfig fstyp fusermount fwctl \
	gpioctl grfconfig gspa \
	hdaudioctl \
	i2cscan ifwatchd inetd installboot intrctl iopctl iostat ipwctl irdaattach \
	isdn isibootd iteconfig iwictl \
	kgmon \
	lastlogin ldpd link lmcconfig lockstat lpr \
	mailwrapper makefs map-mbone mdconfig memswitch mlxctl mmcformat \
	mopd mountd moused mrinfo mrouted mscdlabel mtrace mtree \
	ndbootd ndiscvt netgroup_mkdb nfsd \
	ofctl \
	paxctl pcictl perfused psrset pstat pwd_mkdb postinstall \
	powerd puffs \
	quot quotacheck quotaon quotarestore \
	rarpd rbootd rdate repquota rmt rpc.bootparamd rpc.lockd \
	rpc.pcnfsd rpc.statd rpcbind rwhod \
	sa screenblank sdpd service services_mkdb sesd schedctl \
	sliplogin spray \
	srtconfig sti sunlabel sup sysinst syslogd \
	tadpolectl tcpdchk tcpdmatch tcpdrop timed tpctl tprof traceroute trpt \
	unlink usbdevs user \
	videomode vipw veriexecgen vnconfig \
	wakeonlan wiconfig wlanctl wsconscfg wsfontload wsmoused wsmuxctl \
	zdump zic

.if ${MKMAKEMANDB} != "no"
SUBDIR+= makemandb
.endif

.if (${MKYP} != "no")
SUBDIR+= rpc.yppasswdd ypbind yppoll ypserv ypset
.endif

.if ${TOOLCHAIN_MISSING} != "no"
SUBDIR+= mdsetimage
.endif

.if (${MKINET6} != "no")
SUBDIR+=faithd ifmcstat ip6addrctl ndp rip6query rtsold
.endif
.if (${USE_INET6} != "no")
SUBDIR+=mld6query route6d rtadvd traceroute6 
.endif

# ATM PVC
SUBDIR+=pvcsif pvctxctl

SUBDIR+= racoon racoonctl

# NPF
.if (${MKNPF} != "no")
SUBDIR+=npf
.endif

# PF
.if (${MKPF} != "no")
SUBDIR+=pf
.endif

.include <bsd.subdir.mk>
