#	$NetBSD: Makefile,v 1.298 2025/12/22 17:25:51 thorpej Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton acpitools altq apm apmd arp autofs \
	bad144 bootp catman \
	certctl chroot cpuctl crash \
	dev_mkdb diskpart dumpfs dumplfs \
	edquota eeprom emcfanctl envstat etcupdate extattrctl \
	flashctl fssconfig fstyp fusermount fwctl \
	gpioctl grfconfig gspa \
	i2cscan ifwatchd inetd installboot intrctl iopctl iostat ipwctl irdaattach \
	isibootd iteconfig \
	kgmon \
	lastlogin ldpd link lockstat lpr \
	mailwrapper makefs map-mbone mdconfig memswitch mlxctl mmcformat \
	mopd mountd moused mrinfo mrouted mscdlabel mtrace mtree \
	ndbootd netgroup_mkdb nfsd \
	ofctl \
	paxctl pcictl perfused psrset pstat pwd_mkdb postinstall \
	powerd puffs \
	quot quotacheck quotaon quotarestore \
	rarpd rbootd rdate repquota rmt rpc.bootparamd rpc.lockd \
	rpc.pcnfsd rpc.statd rpcbind rwhod \
	sa screenblank service services_mkdb sesd schedctl \
	sliplogin spray \
	srtconfig sti sunlabel sysinst syslogd \
	tadpolectl tcpdchk tcpdmatch tcpdrop timed tpctl tprof traceroute trpt \
	umcpmioctl unlink usbdevs user \
	videomode vipw veriexecgen vnconfig \
	wakeonlan wg-keygen wg-userspace wgconfig \
	wsconscfg wsfontload wsmoused wsmuxctl \
	zdump zic

.if (${MKAUDIO} != "no")
SUBDIR+= hdaudioctl
.endif

.if (${MKBLUETOOTH} != "no")
SUBDIR+= bta2dpd btattach btconfig btdevctl bthcid btpand sdpd
.endif

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

.if (${MKWLAN} != "no")
SUBDIR+=cnwctl iwictl wiconfig wlanctl
.endif

.include <bsd.subdir.mk>
