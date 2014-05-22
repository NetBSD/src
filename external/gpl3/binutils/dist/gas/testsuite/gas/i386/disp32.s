	.text
	mov (%eax),%ebx
	mov 3(%eax),%ebx

	mov.d8 (%eax),%ebx
	mov.d8 3(%eax),%ebx
	mov.d8 0xfff(%eax),%ebx

	mov.d32 (%eax),%ebx
	mov.d32 3(%eax),%ebx

	jmp foo
	jmp.d8 foo
	jmp.d32 foo
foo:

	.intel_syntax noprefix
	mov DWORD PTR [eax], ebx
	mov DWORD PTR [eax+3], ebx
	mov DWORD PTR [eax+0xfff], ebx

	mov.d8 DWORD PTR [eax], ebx
	mov.d8 DWORD PTR [eax+3], ebx

	mov.d32 DWORD PTR [eax], ebx
	mov.d32 DWORD PTR [eax+3], ebx
