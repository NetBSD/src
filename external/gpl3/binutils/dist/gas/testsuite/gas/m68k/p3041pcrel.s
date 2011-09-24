	lea	mytext(%pc),%a0
	lea	mytext+2(%pc),%a0
	lea	mytext-4(%pc),%a0
	lea	mydata(%pc),%a0
	lea	mydata+3(%pc),%a0
	lea	mydata-1(%pc),%a0
	lea	mybss(%pc),%a0
	lea	mybss+1(%pc),%a0
	lea	mybss-2(%pc),%a0
	.weak	mytext
mytext:
	nop
	nop
	nop
	nop

	.data
	.word	0x8081
	.weak	mydata
mydata:
	.word	0x8283
	.word	0x8485

	.bss
	.skip	6
	.weak	mybss
mybss:
	.skip	2
