#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	$Id: Makefile,v 1.43 1995/04/21 04:42:34 cgd Exp $

# not yet done: catman

SUBDIR=	ac accton arp bootpd bootpgw bootpef bootptest \
	chown chroot config config.new cron dev_mkdb \
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
SUBDIR+=bad144 hilinfo grfinfo
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.elif ${MACHINE} == "hp300"
# probably could be expanded to other machines, too
SUBDIR+=hilinfo grfinfo
.elif ${MACHINE} == "amiga"
SUBDIR+=iteconfig
.elif ${MACHINE} == "sun3"
SUBDIR+=dbsym
.endif

.include <bsd.subdir.mk>
