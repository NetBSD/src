#Test the special case of the index bits, 0x4, in SIB.

	.text
	.allow_index_reg
foo:
	mov	-30,%ebx
	mov	-30(,%eiz),%ebx
	mov	-30(,%eiz,1),%eax
	mov	-30(,%eiz,2),%eax
	mov	-30(,%eiz,4),%eax
	mov	-30(,%eiz,8),%eax
	mov	30,%eax
	mov	30(,%eiz),%eax
	mov	30(,%eiz,1),%eax
	mov	30(,%eiz,2),%eax
	mov	30(,%eiz,4),%eax
	mov	30(,%eiz,8),%eax
	mov	(%ebx),%eax
	mov	(%ebx,%eiz),%eax
	mov	(%ebx,%eiz,1),%eax
	mov	(%ebx,%eiz,2),%eax
	mov	(%ebx,%eiz,4),%eax
	mov	(%ebx,%eiz,8),%eax
	mov	(%esp),%eax
	mov	(%esp,%eiz,1),%eax
	mov	(%esp,%eiz,2),%eax
	mov	(%esp,%eiz,4),%eax
	mov	(%esp,%eiz,8),%eax
	.intel_syntax noprefix
        mov    eax,DWORD PTR [eiz*1-30]
        mov    eax,DWORD PTR [eiz*2-30]
        mov    eax,DWORD PTR [eiz*4-30]
        mov    eax,DWORD PTR [eiz*8-30]
        mov    eax,DWORD PTR [eiz*1+30]
        mov    eax,DWORD PTR [eiz*2+30]
        mov    eax,DWORD PTR [eiz*4+30]
        mov    eax,DWORD PTR [eiz*8+30]
        mov    eax,DWORD PTR [ebx+eiz]
        mov    eax,DWORD PTR [ebx+eiz*1]
        mov    eax,DWORD PTR [ebx+eiz*2]
        mov    eax,DWORD PTR [ebx+eiz*4]
        mov    eax,DWORD PTR [ebx+eiz*8]
        mov    eax,DWORD PTR [esp]
        mov    eax,DWORD PTR [esp+eiz]
        mov    eax,DWORD PTR [esp+eiz*1]
        mov    eax,DWORD PTR [esp+eiz*2]
        mov    eax,DWORD PTR [esp+eiz*4]
        mov    eax,DWORD PTR [esp+eiz*8]
	.p2align 4
