#	$NetBSD: Makefile,v 1.290 2022/05/29 15:23:20 nia Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton acpitools altq apm apmd arp autofs \
	bad144 bootp bta2dpd btattach btconfig btdevctl bthcid btpand catman \
	chroot cnwctl cpuctl crash \
	dev_mkdb diskpart dumpfs \
	edquota eeprom envstat etcupdate extattrctl \
	flashctl fssconfig fstyp fusermount fwctl \
	gpioctl grfconfig gspa \
	hdaudioctl \
	i2cscan ifwatchd inetd installboot intrctl iopctl iostat ipwctl irdaattach \
	isibootd iteconfig iwictl \
	kgmon \
	lastlogin ldpd link lockstat lpr \
	mailwrapper makefs mdconfig memswitch mlxctl mmcformat \
	mopd mountd mscdlabel mtree \
	ndbootd netgroup_mkdb nfsd \
	ofctl \
	paxctl pcictl perfused psrset pstat pwd_mkdb postinstall \
	powerd puffs \
	quot quotacheck quotaon quotarestore \
	rarpd rbootd rdate repquota rmt rpc.bootparamd rpc.lockd \
	rpc.pcnfsd rpc.statd rpcbind rwhod \
	sa screenblank sdpd service services_mkdb sesd schedctl \
	sliplogin spray \
	srtconfig sti sunlabel sysinst syslogd \
	tadpolectl tcpdchk tcpdmatch tcpdrop tpctl tprof traceroute trpt \
	unlink usbdevs user \
	videomode vipw veriexecgen vnconfig \
	wakeonlan wg-keygen wg-userspace wgconfig wiconfig wlanctl \
	wsconscfg wsfontload wsmoused wsmuxctl \
	zdump zic

.if ${MKLFS} != "no"
SUBDIR+= dumplfs
.endif

.if ${MKMAKEMANDB} != "no"
SUBDIR+= makemandb
.endif

.if ${MKMROUTING} != "no"
SUBDIR+= map-mbone mrinfo mrouted mtrace
.endif

.if ${MKMOUSED} != "no"
SUBDIR+= moused
.endif

.if ${MKTIMED} != "no"
SUBDIR+= timed
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

SUBDIR+= plainrsa-gen racoon racoonctl

SUBDIR+= nvmmctl

# NPF
.if (${MKNPF} != "no")
SUBDIR+=npf
.endif

# PF
.if (${MKPF} != "no")
SUBDIR+=pf
.endif

.include <bsd.subdir.mk>
