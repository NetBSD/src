#	$Id: mcount.sed,v 1.2 1993/08/14 13:43:51 mycroft Exp $
s/.word	0x0.*$/&\
	.data\
1:\
	.long	0\
	.text\
	addr	1b,r0\
	jsb	mcount/
