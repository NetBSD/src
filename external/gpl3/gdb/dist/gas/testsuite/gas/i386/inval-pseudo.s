 	.text
	nop
	{disp32}
	nop
	{disp32} movb (%bp),%al
	{disp16} movb (%ebp),%al
	.p2align 4,0
