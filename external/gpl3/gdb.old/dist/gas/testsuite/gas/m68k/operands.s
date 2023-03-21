# Test handling of MIT and Motorola syntax operands
# If you change this file, see also op68000.d.
	.text
foo:	
	| Data register direct
	tstl	%d0

	| Address register direct
	tstl	%a0

	| Address register indirect
	tstl	%a0@
	tstl	(%a0)

	| Address register indirect with postincrement
	tstl	%a0@+
	tstl	(%a0)+

	| Address register indirect with predecrement
	tstl	%a0@-
	tstl	-(%a0)

	| Address register indirect with displacement
	tstl	%a0@(8)
	tstl	(8,%a0)
	tstl	8(%a0)

	| Address register indirect with index (8-bit displacement)
	tstl	%a0@(8,%d0)
	tstl	%a0@(8,%d0:w)
	tstl	%a0@(8,%d0:w:1)
	tstl	%a0@(8,%d0:w:2)
	tstl	%a0@(8,%d0:w:4)
	tstl	%a0@(8,%d0:w:8)
	tstl	%a0@(8,%d0:l)
	tstl	%a0@(8,%d0:l:1)
	tstl	%a0@(8,%d0:l:2)
	tstl	%a0@(8,%d0:l:4)
	tstl	%a0@(8,%d0:l:8)
	tstl	%a0@(%d0:w:2)
	tstl	(8,%a0,%d0)
	tstl	(8,%a0,%d0*1)
	tstl	(8,%a0,%d0*2)
	tstl	(8,%a0,%d0*4)
	tstl	(8,%a0,%d0*8)
	tstl	(8,%a0,%d0.w)
	tstl	(8,%a0,%d0.w*1)
	tstl	(8,%a0,%d0.w*2)
	tstl	(8,%a0,%d0.w*4)
	tstl	(8,%a0,%d0.w*8)
	tstl	(8,%a0,%d0.l)
	tstl	(8,%a0,%d0.l*1)
	tstl	(8,%a0,%d0.l*2)
	tstl	(8,%a0,%d0.l*4)
	tstl	(8,%a0,%d0.l*8)
	tstl	(8,%d0,%a0)
	tstl	(8,%a1.w*2,%a0)
	tstl	(8,%a1,%a0)
	tstl	8(%a0,%d0.w*2)
	tstl	8(%d0.w*2,%a0)
	tstl	8(%a1.w*2,%a0)
	tstl	(%a0,%d0.w*2)
	tstl	(%d0.w*2,%a0)

	| Address register indirect with index (base displacement)
	tstl	%a0@(1000,%d0:w:2)
	tstl	@(1000,%d0:w:2)
	tstl	@(%d0:w:2)
	tstl	@(1000)
	tstl	%a0@(100000)
	tstl	(1000,%a0,%d0.w*2)
	tstl	(1000,%d0,%a0)
	tstl	(1000,%a1.w*2,%a0)
	tstl	1000(%a0,%d0.w*2)
	tstl	1000(%d0,%a0)
	tstl	(1000,%d0.w*2)
	tstl	1000(%d0.w*2)
	tstl	(%d0.w*2)
	tstl	(100000,%a0)
	tstl	100000(%a0)
	tstl	%za1@(1000,%d0:w:2)
	tstl	%za1@(100000)
	tstl	(1000,%za1,%d0.w*2)
	tstl	(1000,%d0,%za1)
	tstl	(1000,%a1.w*2,%za1)
	tstl	1000(%za1,%d0.w*2)
	tstl	1000(%d0,%za1)
	tstl	(100000,%za1)
	tstl	100000(%za1)
	tstl	%a0@(1000,%zd1:w:2)
	tstl	@(1000,%zd1:w:2)
	tstl	@(%zd1:w:2)
	tstl	(1000,%a0,%zd1.w*2)
	tstl	(1000,%zd1,%a0)
	tstl	(1000,%za1.w*2,%a0)
	tstl	1000(%a0,%zd1.w*2)
	tstl	1000(%zd1,%a0)
	tstl	(1000,%zd1.w*2)
	tstl	1000(%zd1.w*2)
	tstl	(%zd1.w*2)

	| Memory indirect postindexed
	tstl	%a0@(1000)@(2000,%d0:w:2)
	tstl	%a0@(1000)@(%d0:w:2)
	tstl	%a0@(1000)@(2000)
	tstl	@(1000)@(2000,%d0:w:2)
	tstl	@(1000)@(%d0:w:2)
	tstl	@(1000)@(2000)
	tstl	%a0@(0)@(2000,%d0:w:2)
	tstl	%a0@(0)@(%d0:w:2)
	tstl	%a0@(0)@(2000)
	tstl	@(0)@(2000,%d0:w:2)
	tstl	@(0)@(%d0:w:2)
	tstl	@(0)@(2000)
	tstl	([1000,%a0],%d0:w:2,2000)
	tstl	([1000,%a0],%d0:w:2)
	tstl	([1000,%a0],2000)
	tstl	([1000],%d0:w:2,2000)
	tstl	([1000],%d0:w:2)
	tstl	([1000],2000)
	tstl	([%a0],%d0:w:2,2000)
	tstl	([%a0],%d0:w:2)
	tstl	([%a0],2000)
	tstl	([0],%d0:w:2,2000)
	tstl	([0],%d0:w:2)
	tstl	([0],2000)

	| Memory indirect preindexed
	tstl	%a0@(1000,%d0:w:2)@(2000)
	tstl	%a0@(1000,%d0:w:2)@(0)
	tstl	@(1000,%d0:w:2)@(2000)
	tstl	@(1000,%d0:w:2)@(0)
	tstl	%a0@(%d0:w:2)@(2000)
	tstl	%a0@(%d0:w:2)@(0)
	tstl	@(%d0:w:2)@(2000)
	tstl	@(%d0:w:2)@(0)
	tstl	([1000,%a0,%d0:w:2],2000)
	tstl	([1000,%d0:w:2,%a0],2000)
	tstl	([1000,%d0,%a0],2000)
	tstl	([1000,%a1,%a0],2000)
	tstl	([1000,%a1:w:2,%a0],2000)
	tstl	([1000,%a0,%d0:w:2])
	tstl	([1000,%d0,%a0])
	tstl	([1000,%d0:w:2],2000)
	tstl	([1000,%d0:w:2])
	tstl	([%a0,%d0:w:2],2000)
	tstl	([%d0,%a0],2000)
	tstl	([%a0,%d0:w:2])
	tstl	([%d0,%a0])
	tstl	([%d0:w:2],2000)
	tstl	([%d0:w:2])

	| Program counter indirect with displacement
	pea	%pc@(8)
	pea	(8,%pc)
	pea	8(%pc)
	pea	foo

	| Program counter indirect with index (8-bit displacement)
	pea	%pc@(8,%d0:w:2)
	pea	%pc@(%d0:w:2)
	pea	(8,%pc,%d0.w*2)
	pea	(8,%d0,%pc)
	pea	(8,%a0,%pc)
	pea	8(%pc,%d0.w*2)
	pea	8(%d0,%pc)
	pea	8(%a0,%pc)
	pea	(%pc,%d0.w*2)
	pea	(%d0,%pc)
	pea	(%a0,%pc)

	| Program counter indirect with index (base displacement)
	pea	%pc@(1000,%d0:w:2)
	pea	%pc@(100000)
	pea	(1000,%pc,%d0.w*2)
	pea	(1000,%d0,%pc)
	pea	(1000,%a1.w*2,%pc)
	pea	(1000,%a1,%pc)
	pea	1000(%pc,%d0.w*2)
	pea	1000(%d0,%pc)
	pea	1000(%a1,%pc)
	pea	(100000,%pc)
	pea	100000(%pc)
	pea	%zpc@(1000,%d0:w:2)
	pea	%zpc@(100000)
	pea	(1000,%zpc,%d0.w*2)
	pea	(1000,%d0,%zpc)
	pea	(1000,%a1.w*2,%zpc)
	pea	(1000,%a1,%zpc)
	pea	1000(%zpc,%d0.w*2)
	pea	1000(%d0,%zpc)
	pea	1000(%a1,%zpc)
	pea	(100000,%zpc)
	pea	100000(%zpc)

	| Program counter memory indirect postindexed
	pea	%pc@(1000)@(2000,%d0:w:2)
	pea	%pc@(1000)@(%d0:w:2)
	pea	%pc@(1000)@(2000)
	pea	%pc@(0)@(2000,%d0:w:2)
	pea	%pc@(0)@(%d0:w:2)
	pea	%pc@(0)@(2000)
	pea	([1000,%pc],%d0:w:2,2000)
	pea	([1000,%pc],%d0:w:2)
	pea	([1000,%pc],2000)
	pea	([%pc],%d0:w:2,2000)
	pea	([%pc],%d0:w:2)
	pea	([%pc],2000)
	pea	%zpc@(1000)@(2000,%d0:w:2)
	pea	%zpc@(1000)@(%d0:w:2)
	pea	%zpc@(1000)@(2000)
	pea	%zpc@(0)@(2000,%d0:w:2)
	pea	%zpc@(0)@(%d0:w:2)
	pea	%zpc@(0)@(2000)
	pea	([1000,%zpc],%d0:w:2,2000)
	pea	([1000,%zpc],%d0:w:2)
	pea	([1000,%zpc],2000)
	pea	([%zpc],%d0:w:2,2000)
	pea	([%zpc],%d0:w:2)
	pea	([%zpc],2000)

	| Program counter memory indirect preindexed
	pea	%pc@(1000,%d0:w:2)@(2000)
	pea	%pc@(1000,%d0:w:2)@(0)
	pea	%pc@(%d0:w:2)@(2000)
	pea	%pc@(%d0:w:2)@(0)
	pea	([1000,%pc,%d0:w:2],2000)
	pea	([1000,%d0:w:2,%pc],2000)
	pea	([1000,%d0,%pc],2000)
	pea	([1000,%a1,%pc],2000)
	pea	([1000,%pc,%a1],2000)
	pea	([1000,%a1:w:2,%pc],2000)
	pea	([1000,%pc,%d0:w:2])
	pea	([1000,%d0,%pc])
	pea	([1000,%a1,%pc])
	pea	([%pc,%d0:w:2],2000)
	pea	([%pc,%a0],2000)
	pea	([%pc,%d0:w:2])
	pea	([%d0,%pc])
	pea	%zpc@(1000,%d0:w:2)@(2000)
	pea	%zpc@(1000,%d0:w:2)@(0)
	pea	%zpc@(%d0:w:2)@(2000)
	pea	%zpc@(%d0:w:2)@(0)
	pea	([1000,%zpc,%d0:w:2],2000)
	pea	([1000,%d0:w:2,%zpc],2000)
	pea	([1000,%d0,%zpc],2000)
	pea	([1000,%a1,%zpc],2000)
	pea	([1000,%zpc,%a1],2000)
	pea	([1000,%a1:w:2,%zpc],2000)
	pea	([1000,%zpc,%d0:w:2])
	pea	([1000,%d0,%zpc])
	pea	([1000,%a1,%zpc])
	pea	([%zpc,%d0:w:2],2000)
	pea	([%zpc,%a0],2000)
	pea	([%zpc,%d0:w:2])
	pea	([%d0,%zpc])

	| Absolute short
	tstl	4
	tstl	4.w
	tstl	(4).w

	| Absolute long
	tstl	100000
	tstl	8.l
	tstl	(8).l

	| Immediate
	addib	&1,%d0
	addiw	&1,%d0
	addil	&1,%d0
	addqb	&1,%d0

	| cmpi
	cmpib	&1,%d0
	cmpib	&1,0(%pc)
	cmpiw	&1,%d0
	cmpiw	&1,0(%pc)
	cmpil	&1,%d0
	cmpil	&1,0(%pc)
	cmpb	&1,%d0
	cmpb	&1,0(%pc)
	cmpw	&1,%d0
	cmpw	&1,0(%pc)
	cmpl	&1,%d0
	cmpl	&1,0(%pc)

