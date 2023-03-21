	.data
	.space	0x1000

	.align	2
	.type	bar, @object
bar:
	.half	bar
	.size	bar, . - bar
