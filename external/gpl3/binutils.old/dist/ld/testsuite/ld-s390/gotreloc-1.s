	.text
	.globl foo
foo:
	lgrl	%r1,bar@GOTENT
	lg	%r1,bar@GOT(%r12)
	lrl	%r1,bar@GOTENT
	l	%r1,bar@GOT(%r12)
	ly	%r1,bar@GOT(%r12)

.globl	bar
bar:	.long	0x123
