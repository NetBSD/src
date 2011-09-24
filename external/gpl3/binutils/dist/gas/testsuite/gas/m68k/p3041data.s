	.text
	nop
	nop
	nop
	.weak	mytext
mytext:
	nop

	.data
	.long	0x12345678
	.long	mytext
	.long	mytext+1
	.weak	mydata
mydata:
	.long	mytext-3
	.long	mydata
	.long	mydata+3
	.long	mydata-2
	.long	mybss
	.long	mybss+2
	.long	mybss-1

	.bss
	.skip	16
	.weak	mybss
mybss:
	.word	1
