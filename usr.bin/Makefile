#	$NetBSD: Makefile,v 1.77.2.4 1998/11/22 03:13:31 cgd Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

SUBDIR=	apply apropos asa at audioctl banner basename bdes \
	biff cal calendar cap_mkdb checknr chflags chpass \
	cksum cmp col colcrt colrm column comm compress \
	crontab crunch ctags cut dirname du eject env \
	error expand false fdformat file find finger fmt fold fpr \
	from fsplit fstat ftp gencat getconf getopt gprof head \
	hexdump id indent ipcrm ipcs join jot kdump ktrace \
	lam last lastcomm leave less lex locate lock \
	logger login logname look lorder m4 machine mail \
	make man menuc mesg mixerctl mkdep mkfifo mkstr modstat \
	msgc msgs netstat newsyslog nfsstat nice nohup pagesize \
	passwd paste patch pr printenv printf quota rdist \
	renice rev rlogin rpcgen rpcinfo rs rsh rup \
	ruptime rusers rwall rwho script sed shar shlock \
	showmount skey skeyinfo skeyinit soelim split \
	strings su systat tail talk tcopy tee tftp time \
	tip tn3270 touch tput tr true tset tsort tty ul \
	uname unexpand unifdef uniq units unvis users \
	uudecode uuencode vacation vgrind vi vis vmstat w \
	wall wc what whatis whereis which who whois window \
	write xargs xinstall xlint xstr yacc yes ypcat \
	ypmatch ypwhich

.if	make(clean) || make(cleandir)
SUBDIR+=ar elf2aout elf2ecoff ldd nm ranlib size strip telnet
.else

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/usr.bin/telnet) || \
    defined(EXPORTABLE_SYSTEM)
SUBDIR+=telnet
.endif

# Don't use the standard a.out tool chain on non-aout (ELF) platforms.
# On ELF platforms, build the  ELF ldd.
.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "mips" && \
     ${MACHINE_ARCH} != "powerpc")
SUBDIR+= ar nm ranlib size strip
.else
SUBDIR+= ldd
.endif

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mips")
SUBDIR+= elf2aout elf2ecoff
.endif

.endif	# make(clean) || make(cleandir)

.include <bsd.subdir.mk>
