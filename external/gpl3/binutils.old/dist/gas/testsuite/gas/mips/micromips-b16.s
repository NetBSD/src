	.text
test1:
	.space	65536
test2:
	b16	1f
1:
	bnez16	$2,1f
1:
	beqz16	$2,1f
1:
	b	1f
1:
	bnez	$2,1f
1:
	beqz	$2,1f
1:
	nop
