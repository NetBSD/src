#	$NetBSD: Makefile,v 1.138 2004/03/26 08:52:26 mrg Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= apply apropos asa at audio awk banner basename biff \
	bzip2 bzip2recover cal calendar cap_mkdb cdplay \
	checknr chflags chpass cksum cmp col colcrt colrm \
	column comm compress crontab crunch ctags cut \
	db dirname du eject elf2aout elf2ecoff env error expand \
	false fdformat fgen file find finger fmt fold fpr from \
	fsplit fstat ftp gcore gencat getconf getopt gprof gzip \
	head hexdump iconv id indent innetgr ipcrm ipcs join jot \
	kdump ktrace ktruss lam last lastcomm ldd leave less \
	lex locale locate lock logger login logname look lorder m4 \
	machine mail make man menuc mesg midiplay mixerctl mkcsmapper \
	mkdep mkesdb mkfifo mklocale mkstr mktemp modstat msgc msgs \
	netgroup netstat newsyslog nfsstat nice nl nohup \
	pagesize passwd paste patch pathchk pkill pmap pmc pr \
	printenv printf progress pwhash qsubst quota radioctl rdist \
	renice rev rlogin rpcgen rpcinfo rs rsh rup ruptime \
	rusers rwall rwho script sed shar shlock showmount \
	shuffle soelim sort split stat su systat tail talk \
	tcopy tee telnet tftp time tip tn3270 top touch tput \
	tr true tset tsort tty ul uname unexpand unifdef \
	uniq units unvis usbhidaction usbhidctl users \
	uudecode uuencode vacation vgrind vi vis vmstat w \
	wall wc what whatis whereis which who whois window \
	write xargs xinstall xlint xstr yacc yes

.if (${MKHESIOD} != "no")
SUBDIR+= hesinfo
.endif

.if (${MKKERBEROS} != "no")
SUBDIR+= compile_et kdestroy kinit klist mk_cmds string2key
.endif

.if (${MKSKEY} != "no")
SUBDIR+= skey skeyinfo skeyinit
.endif

.if (${MKYP} != "no")
SUBDIR+= ypcat ypmatch ypwhich
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+= openssl
SUBDIR+= bdes
SUBDIR+= ssh
.endif

.include <bsd.subdir.mk>
