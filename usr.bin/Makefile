#	$NetBSD: Makefile,v 1.154 2006/10/07 08:48:03 elad Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= apply apropos asa at audio awk banner basename biff bthset btpin \
	bzip2 bzip2recover cal calendar cap_mkdb cdplay \
	checknr chflags chpass cksum cmp col colcrt colrm \
	column comm compress config crontab crunch csplit ctags cut \
	deroff db dirname du \
	eject elf2aout elf2ecoff env error expand extattr \
	false fdformat fgen file find finger fmt fold fpr from \
	fsplit fstat ftp gcore genassym gencat getconf getent getopt gprof \
	gzip head hexdump iconv id indent innetgr ipcrm ipcs join jot \
	kdump ktrace ktruss lam last lastcomm ldd leave less \
	lex locale locate lock logger login logname look lorder m4 \
	machine mail make man menuc mesg midiplay mixerctl mkcsmapper \
	mkdep mkesdb mkfifo mklocale mkstr mktemp modstat moduli msgc msgs \
	netgroup netstat newsyslog nfsstat nice nl nohup \
	pagesize passwd paste patch pathchk paxctl pkill pmap pmc pr \
	printenv printf progress pwhash qsubst quota radioctl rdist \
	renice rev revoke rfcomm_sppd rlogin rpcgen rpcinfo rs rsh rup
	ruptime \
	rusers rwall rwho script sdpquery sed seq shar shlock showmount \
	shuffle sockstat soelim sort spell split stat su systat tail talk \
	tcopy tee telnet tftp time tip tn3270 top touch tput \
	tr true tset tsort tty ul uname unexpand unifdef \
	uniq units unvis usbhidaction usbhidctl users utoppya \
	uudecode uuencode uuidgen vacation vgrind vi vis vmstat vndcompress w \
	wall wc what whatis whereis who whois window \
	write xargs xinstall xlint xstr yacc yes

.if (${MKHESIOD} != "no")
SUBDIR+= hesinfo
.endif

.if (${MKKERBEROS} != "no")
SUBDIR+= compile_et kdestroy kgetcred kinit klist mk_cmds string2key
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
