#	$NetBSD: majors.arc,v 1.14.2.4 2008/01/21 09:35:32 yamt Exp $
#
# Device majors for arc
#

device-major	cons		char 0
device-major	swap		char 1   block 1	vmswap
device-major	ctty		char 2
device-major	mem		char 3
device-major	pts		char 4			pty
device-major	ptc		char 5			pty
device-major	log		char 6
device-major	filedesc	char 7
device-major	cd		char 8   block 3	cd
device-major	sd		char 9   block 0	sd
device-major	st		char 10  block 5	st
device-major	vnd		char 11  block 2	vnd
device-major	bpf		char 12			bpfilter
device-major	fd		char 13  block 7	fdc
device-major	pc		char 14			pc
device-major	opms		char 15			opms
device-major	lpt		char 16			lpt
device-major	com		char 17			com
device-major	wd		char 18  block 4	wd
device-major	scsibus		char 19			scsibus

device-major	md		char 22  block 8	md
device-major	ccd		char 23  block 6	ccd
device-major	raid		char 24  block 9	raid
device-major	tun		char 25			tun
device-major	joy		char 26			joy
device-major	wsdisplay	char 27			wsdisplay
device-major	wsmux		char 28			wsmux
device-major	wskbd		char 29			wskbd
device-major	wsmouse		char 30			wsmouse
device-major	ipl		char 31			ipfilter
device-major	uk		char 32			uk
device-major	rnd		char 33			rnd
device-major	ss		char 34			ss
device-major	ses		char 35			ses
device-major	ch		char 36			ch
device-major	isdn		char 37			isdn
device-major	isdnctl		char 38			isdnctl
device-major	isdnbchan	char 39			isdnbchan
device-major	isdntrc		char 40			isdntrc
device-major	isdntel		char 41			isdntel

device-major	lkm		char 51			lkm
device-major	clockctl	char 52			clockctl
device-major	cgd		char 54  block 10	cgd
device-major	ksyms		char 55			ksyms
device-major	wsfont		char 56			wsfont
device-major	ld		char 57  block 11	ld
device-major	icp		char 58			icp
device-major	mlx		char 59			mlx
device-major	twe		char 60			twe

device-major	nsmb		char 98			nsmb

# Majors up to 143 are reserved for machine-dependant drivers.
# New machine-independent driver majors are assigned in 
# sys/conf/majors.
