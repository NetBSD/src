#	$NetBSD: Makefile,v 1.117 2001/01/13 14:11:38 simonb Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR=	apply apropos asa at audio banner basename biff \
	bzip2 bzip2recover cal calendar cap_mkdb cdplay checknr chflags \
	chpass cksum cmp col colcrt colrm column comm compress \
	crontab crunch ctags cut dirname du eject elf2aout elf2ecoff env \
	error expand false fdformat fgen file find finger fmt fold fpr \
	from fsplit fstat ftp gencat getconf getopt gprof head hesinfo \
	hexdump id indent innetgr ipcrm ipcs join jot kdump ktrace ktruss \
	lam last lastcomm ldd leave less lex locate lock \
	logger login logname look lorder m4 machine mail \
	make man menuc mesg midiplay mixerctl mkdep mkfifo \
	mklocale mkstr mktemp \
	modstat msgc msgs netgroup netstat newsyslog nfsstat nice nl nohup \
	pagesize passwd paste patch pmc pr printenv printf quota rdist \
	renice rev rlogin rpcgen rpcinfo rs rsh rup \
	ruptime rusers rwall rwho script sed shar shlock \
	showmount shuffle skey skeyinfo skeyinit soelim split \
	su systat tail talk tcopy tee telnet tftp time \
	tip tn3270 top touch tput tr true tset tsort tty ul \
	uname unexpand unifdef uniq units unvis usbhidaction usbhidctl users \
	uudecode uuencode vacation vgrind vi vis vmstat w \
	wall wc what whatis whereis which who whois window \
	write xargs xinstall xlint xstr yacc yes ypcat \
	ypmatch ypwhich
#SUBDIR+= sort

.if (${MKCRYPTO} != "no")
SUBDIR+= openssl
SUBDIR+= bdes compile_et kdestroy kf kinit klist kpasswd mk_cmds string2key
SUBDIR+= ssh
.endif

.include <bsd.subdir.mk>
