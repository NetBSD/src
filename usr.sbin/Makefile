#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	$Id: Makefile,v 1.22 1994/01/08 13:23:52 deraadt Exp $

SUBDIR=	accton amd arp chown chroot config config.new dbsym dev_mkdb \
	diskpart edquota flcopy gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mtree named portmap pppd pwd_mkdb quotaon rarpd repquota \
	rmt rpc.bootparamd rwhod sendmail sliplogin slstats spray swapinfo \
	syslogd tcpdump timed traceroute trpt trsp update vipw vnconfig yp

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
