#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	$Id: Makefile,v 1.45 1995/04/28 08:14:41 cgd Exp $

# not yet done: catman

SUBDIR=	ac accton arp bootpd bootpgw bootpef bootptest \
	chown chroot config cron dev_mkdb \
	diskpart edquota gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mrouted mtree named netgroup_mkdb portmap \
	pppd pstat pwd_mkdb quot quotaon \
	rarpd rbootd rdate repquota rmt rpc.bootparamd rwhod \
	sa sendmail sliplogin slstats spray sysctl \
	syslogd tcpdump timed traceroute trpt trsp update vipw vnconfig \
	ypbind yppoll ypset zdump zic

# should be listed above, but doesn't work on the Alpha.
.if   (${MACHINE_ARCH} != "alpha") || make(clean) || make(cleandir)
SUBDIR+=amd
.endif

.if   make(clean) || make(cleandir)
SUBDIR+=bad144 config.old hilinfo grfinfo
.elif ${MACHINE} == "amiga"
SUBDIR+=iteconfig
.elif ${MACHINE} == "hp300"
SUBDIR+=config.old hilinfo grfinfo
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.elif ${MACHINE} == "pmax"
SUBDIR+=config.old
.elif ${MACHINE} == "sun3"
SUBDIR+=dbsym
.endif

.include <bsd.subdir.mk>
