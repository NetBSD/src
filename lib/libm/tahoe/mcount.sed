#	$Id: mcount.sed,v 1.1 1993/08/14 13:44:00 mycroft Exp $
s/.word	0x.*$/&\
	.data\
	.align 2\
9:	.long 0\
	.text\
	pushal	9b\
	callf	\$8,mcount/
