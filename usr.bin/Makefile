#	$NetBSD: Makefile,v 1.93 1998/12/17 15:33:57 pk Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition
#					# and OBJECT_FMT definition

SUBDIR=	apply apropos asa at audioctl banner basename bdes biff \
	bzip2 bzip2recover cal calendar cap_mkdb checknr chflags chpass \
	cksum cmp col colcrt colrm column comm compress \
	crontab crunch ctags cut dirname du eject env \
	error expand false fdformat fgen file find finger fmt fold fpr \
	from fsplit fstat ftp gencat getconf getopt gprof head \
	hexdump id indent ipcrm ipcs join jot kdump ktrace \
	lam last lastcomm ldd leave less lex locate lock \
	logger login logname look lorder m4 machine mail \
	make man menuc mesg midiplay mixerctl mkdep mkfifo mkstr modstat \
	msgc msgs netstat newsyslog nfsstat nice nohup pagesize \
	passwd paste patch pr printenv printf quota rdist \
	renice rev rlogin rpcgen rpcinfo rs rsh rup \
	ruptime rusers rwall rwho script sed shar shlock \
	showmount shuffle skey skeyinfo skeyinit soelim split \
	su systat tail talk tcopy tee telnet tftp time \
	tip tn3270 touch tput tr true tset tsort tty ul \
	uname unexpand unifdef uniq units unvis usbhidctl users \
	uudecode uuencode vacation vgrind vi vis vmstat w \
	wall wc what whatis whereis which who whois window \
	write xargs xinstall xlint xstr yacc yes ypcat \
	ypmatch ypwhich

.if	make(clean) || make(cleandir) || make(distclean)
SUBDIR+=elf2aout elf2ecoff strip telnet
.else

# Don't use the standard a.out tool chain on non-aout (ELF) platforms.
.if (${OBJECT_FMT} != "ELF")
SUBDIR+= strip
.endif

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mips")
SUBDIR+= elf2aout elf2ecoff
.endif

.endif	# make(clean) || make(cleandir) || make(distclean)

.include <bsd.subdir.mk>
