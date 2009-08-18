.psize 0
.text
.extern xxx

1:	jmp	1b
	jmp	xxx
	jmp	*xxx
	jmp	*%edi
	jmp	*(%edi)
	ljmp	*xxx(,%edi,4)
	ljmpw	*xxx(,%edi,4)
	ljmp	*xxx
	ljmpw	*xxx
	ljmp	$0x1234,$xxx

	call	1b
	call	xxx
	call	*xxx
	call	*%edi
	call	*(%edi)
	lcall	*xxx(,%edi,4)
	lcallw	*xxx(,%edi,4)
	lcall	*xxx
	lcallw	*xxx
	lcall	$0x1234,$xxx

	.intel_syntax noprefix
	call	word ptr [ebx]
	call	dword ptr [ebx]
	call	fword ptr [ebx]
	jmp	word ptr [ebx]
	jmp	dword ptr [ebx]
	jmp	fword ptr [ebx]
