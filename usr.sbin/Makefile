#	$NetBSD: Makefile,v 1.84 1997/06/09 16:45:01 agc Exp $
#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91

# not yet done: catman

SUBDIR=	ac accton amd arp bootpd bootpgw bootpef bootptest chown \
	chroot chrtbl config cron dev_mkdb dhcp diskpart edquota gettable \
	gspa htable inetd iostat ipf kgmon kvm_mkdb lpr lastlogin \
	map-mbone mdconfig mdsetimage mopd mountd mrinfo mrouted mtrace \
	mtree nfsd nfsiod named netgroup_mkdb pkg_install portmap pppd \
	pstat pwd_mkdb quot quotaon rarpd rbootd rdate repquota rmt \
	rpc.bootparamd rpc.lockd rpc.pcnfsd rpc.statd rpc.yppasswdd rwhod \
	sa sendmail sliplogin slstats spray sysctl syslogd tcpdchk tcpdmatch \
	tcpdump timed traceroute trpt trsp update vipw vnconfig xntp ypbind \
	yppoll ypserv ypset zdump zic

.if   make(clean) || make(cleandir)
SUBDIR+=apm apmd bad144 dbsym eeprom grfconfig grfinfo hilinfo \
	iteconfig screenblank videomode
.elif ${MACHINE} == "alpha"
SUBDIR+=bad144
.elif ${MACHINE} == "amiga"
SUBDIR+=grfconfig iteconfig screenblank videomode
.elif ${MACHINE} == "atari"
SUBDIR+=iteconfig
.elif ${MACHINE} == "hp300"
SUBDIR+=grfinfo hilinfo
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
