#	$Id: mcount.sed,v 1.1 1993/08/14 13:44:12 mycroft Exp $
s/.word	0x0.*$/&\
	.data\
1:\
	.long	0\
	.text\
	moval	1b,r0\
	jsb	mcount/
