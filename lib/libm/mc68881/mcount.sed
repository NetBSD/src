#	$Id: mcount.sed,v 1.1 1993/08/14 13:43:41 mycroft Exp $
s/^\(_[a-z_].*\):$/&\
	.data\
X\1:\
	.long	0\
	.text\
	movel	#X\1,a0\
	jsr	mcount/
