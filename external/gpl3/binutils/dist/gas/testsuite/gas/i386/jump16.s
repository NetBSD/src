.psize 0
.text
.extern xxx

.code16gcc
1:	jmp	1b
	jmp	xxx
	jmp	*xxx
	jmp	*%edi
	jmp	*(%edi)
	ljmp	*xxx(%edi)
	ljmp	*xxx
	ljmp	$0x1234,$xxx

	call	1b
	call	xxx
	call	*xxx
	call	*%edi
	call	*(%edi)
	lcall	*xxx(%edi)
	lcall	*xxx
	lcall	$0x1234,$xxx

.code16
	jmp	1b
	jmp	*xxx
	jmp	*%di
	jmp	*(%di)
	ljmp	*xxx(%di)
	ljmpl	*xxx(%di)
	ljmp	*xxx
	ljmpl	*xxx
	ljmp	$0x1234,$xxx

	call	1b
	call	xxx
	call	*xxx
	call	*%di
	call	*(%di)
	lcall	*xxx(%di)
	lcalll	*xxx(%di)
	lcall	*xxx
	lcalll	*xxx
	lcall	$0x1234,$xxx

	.intel_syntax noprefix
	call	word ptr [bx]
	call	dword ptr [bx]
	call	fword ptr [bx]
	jmp	word ptr [bx]
	jmp	dword ptr [bx]
	jmp	fword ptr [bx]
