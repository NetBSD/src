# Test instructions not present on C674x.
.text
.nocmp
.globl f
f:
	cmtl .D2T2 *b0,b1
	ll .D2T2 *b0,b1
	sl .D2T2 b0,*b1
.atomic
	cmtl .D2T2 *b0,b1
	ll .D2T2 *b0,b1
	sl .D2T2 b0,*b1
.noatomic
	cmtl .D2T2 *b0,b1
	ll .D2T2 *b0,b1
	sl .D2T2 b0,*b1
