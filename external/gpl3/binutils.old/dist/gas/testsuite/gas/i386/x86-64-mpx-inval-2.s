# MPX instructions
	.allow_index_reg
	.text

	### bndmk
	bndmk (%eax), %bnd1
	bndmk 0x3(%ecx,%ebx,1), %bnd1

	### bndmov
	bndmov (%r8d), %bnd1
	bndmov 0x3(%r9d,%edx,1), %bnd1

	bndmov %bnd1, (%eax)
	bndmov %bnd1, 0x3(%ecx,%eax,1)

	### bndcl
	bndcl (%ecx), %bnd1
	bndcl 0x3(%ecx,%eax,1), %bnd1

	### bndcu
	bndcu (%ecx), %bnd1
	bndcu 0x3(%ecx,%eax,1), %bnd1

	### bndcn
	bndcn (%ecx), %bnd1
	bndcn 0x3(%ecx,%eax,1), %bnd1

	### bndstx
	bndstx %bnd0, 0x3(%eax,%ebx,1)
	bndstx %bnd2, 3(%ebx,1)

	### bndldx
	bndldx 0x3(%eax,%ebx,1), %bnd0
	bndldx 3(%ebx,1), %bnd2

.intel_syntax noprefix
	bndmk bnd1, [eax]
	bndmk bnd1, [edx+1*eax+0x3]

	### bndmov
	bndmov bnd1, [eax]
	bndmov bnd1, [edx+1*eax+0x3]

	bndmov [eax], bnd1
	bndmov [edx+1*eax+0x3], bnd1

	### bndcl
	bndcl bnd1, [eax]
	bndcl bnd1, [edx+1*eax+0x3]

	### bndcu
	bndcu bnd1, [eax]
	bndcu bnd1, [edx+1*eax+0x3]

	### bndcn
	bndcn bnd1, [eax]
	bndcn bnd1, [edx+1*eax+0x3]

	### bndstx
	bndstx [eax+ebx*1+0x3], bnd0
	bndstx [1*ebx+3], bnd2

	### bndldx
	bndldx bnd0, [eax+ebx*1+0x3]
	bndldx bnd2, [1*ebx+3]
