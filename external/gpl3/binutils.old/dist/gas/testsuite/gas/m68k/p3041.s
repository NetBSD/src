	lea	mytext,%a0
	lea	mytext+2,%a0
	lea	mytext-4,%a0
	lea	mydata,%a0
	lea	mydata+3,%a0
	lea	mydata-1,%a0
	lea	mybss,%a0
	lea	mybss+1,%a0
	lea	mybss-2,%a0
	.weak	mytext
mytext:
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
