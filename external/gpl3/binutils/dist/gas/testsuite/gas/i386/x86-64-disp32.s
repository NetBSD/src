	.text
	mov 3(%rax),%ebx
	mov.d32 3(%rax),%ebx

	jmp foo
	jmp.d32 foo
foo:

	.intel_syntax noprefix
	mov DWORD PTR [rax+3], ebx
	mov.d32 DWORD PTR [rax+3], ebx
