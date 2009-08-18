	.text
	.code32
start:
	movsb	(%esi), (%di)
	movsb	(%si), (%edi)
	movsb	(%esi), %ds:(%edi)
	stosb	%ds:(%edi)
	cmpsb	%ds:(%edi), (%esi)
	scasb	%ds:(%edi)
	insb	(%dx), %ds:(%edi)

	.intel_syntax noprefix

	movs	byte ptr [edi], [si]
	movs	byte ptr [di], [esi]
	movs	byte ptr ds:[edi], [esi]
	movs	byte ptr [edi], word ptr [esi]
	stos	byte ptr ds:[edi]
	cmps	byte ptr [esi], ds:[edi]
	cmps	byte ptr [esi], dword ptr [edi]
	scas	byte ptr ds:[edi]
	ins	byte ptr ds:[edi], dx
