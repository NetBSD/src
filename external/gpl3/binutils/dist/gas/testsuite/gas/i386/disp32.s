	.text
	mov 3(%eax),%ebx
	mov.d32 3(%eax),%ebx

	jmp foo
	jmp.d32 foo
foo:

	.intel_syntax noprefix
	mov DWORD PTR [eax+3], ebx
	mov.d32 DWORD PTR [eax+3], ebx
