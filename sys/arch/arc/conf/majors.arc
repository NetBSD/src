#
# Device majors for arc
#

device-major	cons		char 0
device-major	swap		char 1   block 1
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

device-major	ch		char 36			ch
device-major	isdn		char 37			isdn
device-major	isdnctl		char 38			isdnctl
device-major	isdnbchan	char 39			isdnbchan
device-major	isdntrc		char 40			isdntrc
device-major	isdntel		char 41			isdntel

device-major	clockctl	char 52			clockctl
