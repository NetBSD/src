#	@(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#
#	$Header: /cvsroot/src/usr.sbin/Makefile,v 1.10 1993/06/03 19:34:02 brezak Exp $
#

SUBDIR=	accountant accton arp chroot config dbsym dev_mkdb diskpart \
	edquota flcopy gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mtree named portmap pwd_mkdb quotaon repquota \
	rmt rwhod sendmail sliplogin swapinfo syslogd timed \
	traceroute trpt trsp update vipw yp

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
