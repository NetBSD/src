.text	
.ascii	"AES for Intel AES-NI, CRYPTOGAMS by <appro@openssl.org>\0"
.globl	aesni_encrypt
.type	aesni_encrypt,@function
.align	16
aesni_encrypt:
	movups	(%rdi),%xmm0
	movl	240(%rdx),%eax
	movaps	(%rdx),%xmm4
	movaps	16(%rdx),%xmm5
	leaq	32(%rdx),%rdx
	pxor	%xmm4,%xmm0
.Loop_enc1_1:
	aesenc	%xmm5,%xmm0
	decl	%eax
	movaps	(%rdx),%xmm5
	leaq	16(%rdx),%rdx
	jnz	.Loop_enc1_1	
	aesenclast	%xmm5,%xmm0
	movups	%xmm0,(%rsi)
	repz
	retq
.size	aesni_encrypt,.-aesni_encrypt

.globl	aesni_decrypt
.type	aesni_decrypt,@function
.align	16
aesni_decrypt:
	movups	(%rdi),%xmm0
	movl	240(%rdx),%eax
	movaps	(%rdx),%xmm4
	movaps	16(%rdx),%xmm5
	leaq	32(%rdx),%rdx
	pxor	%xmm4,%xmm0
.Loop_dec1_2:
	aesdec	%xmm5,%xmm0
	decl	%eax
	movaps	(%rdx),%xmm5
	leaq	16(%rdx),%rdx
	jnz	.Loop_dec1_2	
	aesdeclast	%xmm5,%xmm0
	movups	%xmm0,(%rsi)
	repz
	retq
.size	aesni_decrypt, .-aesni_decrypt
.type	_aesni_encrypt3,@function
.align	16
_aesni_encrypt3:
	movaps	(%rcx),%xmm4
	shrl	$1,%eax
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
	pxor	%xmm4,%xmm1
	pxor	%xmm4,%xmm2

.Lenc_loop3:
	aesenc	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesenc	%xmm5,%xmm1
	decl	%eax
	aesenc	%xmm5,%xmm2
	aesenc	%xmm4,%xmm0
	movaps	16(%rcx),%xmm5
	aesenc	%xmm4,%xmm1
	leaq	32(%rcx),%rcx
	aesenc	%xmm4,%xmm2
	jnz	.Lenc_loop3

	aesenc	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesenc	%xmm5,%xmm1
	aesenc	%xmm5,%xmm2
	aesenclast	%xmm4,%xmm0
	aesenclast	%xmm4,%xmm1
	aesenclast	%xmm4,%xmm2
	repz
	retq
.size	_aesni_encrypt3,.-_aesni_encrypt3
.type	_aesni_decrypt3,@function
.align	16
_aesni_decrypt3:
	movaps	(%rcx),%xmm4
	shrl	$1,%eax
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
	pxor	%xmm4,%xmm1
	pxor	%xmm4,%xmm2

.Ldec_loop3:
	aesdec	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesdec	%xmm5,%xmm1
	decl	%eax
	aesdec	%xmm5,%xmm2
	aesdec	%xmm4,%xmm0
	movaps	16(%rcx),%xmm5
	aesdec	%xmm4,%xmm1
	leaq	32(%rcx),%rcx
	aesdec	%xmm4,%xmm2
	jnz	.Ldec_loop3

	aesdec	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesdec	%xmm5,%xmm1
	aesdec	%xmm5,%xmm2
	aesdeclast	%xmm4,%xmm0
	aesdeclast	%xmm4,%xmm1
	aesdeclast	%xmm4,%xmm2
	repz
	retq
.size	_aesni_decrypt3,.-_aesni_decrypt3
.type	_aesni_encrypt4,@function
.align	16
_aesni_encrypt4:
	movaps	(%rcx),%xmm4
	shrl	$1,%eax
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
	pxor	%xmm4,%xmm1
	pxor	%xmm4,%xmm2
	pxor	%xmm4,%xmm3

.Lenc_loop4:
	aesenc	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesenc	%xmm5,%xmm1
	decl	%eax
	aesenc	%xmm5,%xmm2
	aesenc	%xmm5,%xmm3
	aesenc	%xmm4,%xmm0
	movaps	16(%rcx),%xmm5
	aesenc	%xmm4,%xmm1
	leaq	32(%rcx),%rcx
	aesenc	%xmm4,%xmm2
	aesenc	%xmm4,%xmm3
	jnz	.Lenc_loop4

	aesenc	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesenc	%xmm5,%xmm1
	aesenc	%xmm5,%xmm2
	aesenc	%xmm5,%xmm3
	aesenclast	%xmm4,%xmm0
	aesenclast	%xmm4,%xmm1
	aesenclast	%xmm4,%xmm2
	aesenclast	%xmm4,%xmm3
	repz
	retq
.size	_aesni_encrypt4,.-_aesni_encrypt4
.type	_aesni_decrypt4,@function
.align	16
_aesni_decrypt4:
	movaps	(%rcx),%xmm4
	shrl	$1,%eax
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
	pxor	%xmm4,%xmm1
	pxor	%xmm4,%xmm2
	pxor	%xmm4,%xmm3

.Ldec_loop4:
	aesdec	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesdec	%xmm5,%xmm1
	decl	%eax
	aesdec	%xmm5,%xmm2
	aesdec	%xmm5,%xmm3
	aesdec	%xmm4,%xmm0
	movaps	16(%rcx),%xmm5
	aesdec	%xmm4,%xmm1
	leaq	32(%rcx),%rcx
	aesdec	%xmm4,%xmm2
	aesdec	%xmm4,%xmm3
	jnz	.Ldec_loop4

	aesdec	%xmm5,%xmm0
	movaps	(%rcx),%xmm4
	aesdec	%xmm5,%xmm1
	aesdec	%xmm5,%xmm2
	aesdec	%xmm5,%xmm3
	aesdeclast	%xmm4,%xmm0
	aesdeclast	%xmm4,%xmm1
	aesdeclast	%xmm4,%xmm2
	aesdeclast	%xmm4,%xmm3
	repz
	retq
.size	_aesni_decrypt4,.-_aesni_decrypt4
.globl	aesni_ecb_encrypt
.type	aesni_ecb_encrypt,@function
.align	16
aesni_ecb_encrypt:
	cmpq	$16,%rdx
	jb	.Lecb_ret

	movl	240(%rcx),%eax
	andq	$-16,%rdx
	movq	%rcx,%r11
	testl	%r8d,%r8d
	movl	%eax,%r10d
	jz	.Lecb_decrypt

	subq	$64,%rdx
	jbe	.Lecb_enc_tail
	jmp	.Lecb_enc_loop3
.align	16
.Lecb_enc_loop3:
	movups	(%rdi),%xmm0
	movups	16(%rdi),%xmm1
	movups	32(%rdi),%xmm2
	call	_aesni_encrypt3
	subq	$48,%rdx
	leaq	48(%rdi),%rdi
	leaq	48(%rsi),%rsi
	movups	%xmm0,-48(%rsi)
	movl	%r10d,%eax
	movups	%xmm1,-32(%rsi)
	movq	%r11,%rcx
	movups	%xmm2,-16(%rsi)
	ja	.Lecb_enc_loop3

.Lecb_enc_tail:
	addq	$64,%rdx
	jz	.Lecb_ret

	cmpq	$16,%rdx
	movups	(%rdi),%xmm0
	je	.Lecb_enc_one
	cmpq	$32,%rdx
	movups	16(%rdi),%xmm1
	je	.Lecb_enc_two
	cmpq	$48,%rdx
	movups	32(%rdi),%xmm2
	je	.Lecb_enc_three
	movups	48(%rdi),%xmm3
	call	_aesni_encrypt4
	movups	%xmm0,(%rsi)
	movups	%xmm1,16(%rsi)
	movups	%xmm2,32(%rsi)
	movups	%xmm3,48(%rsi)
	jmp	.Lecb_ret
.align	16
.Lecb_enc_one:
	movaps	(%rcx),%xmm4
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
.Loop_enc1_3:
	aesenc	%xmm5,%xmm0
	decl	%eax
	movaps	(%rcx),%xmm5
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_3	
	aesenclast	%xmm5,%xmm0
	movups	%xmm0,(%rsi)
	jmp	.Lecb_ret
.align	16
.Lecb_enc_two:
	call	_aesni_encrypt3
	movups	%xmm0,(%rsi)
	movups	%xmm1,16(%rsi)
	jmp	.Lecb_ret
.align	16
.Lecb_enc_three:
	call	_aesni_encrypt3
	movups	%xmm0,(%rsi)
	movups	%xmm1,16(%rsi)
	movups	%xmm2,32(%rsi)
	jmp	.Lecb_ret

.align	16
.Lecb_decrypt:
	subq	$64,%rdx
	jbe	.Lecb_dec_tail
	jmp	.Lecb_dec_loop3
.align	16
.Lecb_dec_loop3:
	movups	(%rdi),%xmm0
	movups	16(%rdi),%xmm1
	movups	32(%rdi),%xmm2
	call	_aesni_decrypt3
	subq	$48,%rdx
	leaq	48(%rdi),%rdi
	leaq	48(%rsi),%rsi
	movups	%xmm0,-48(%rsi)
	movl	%r10d,%eax
	movups	%xmm1,-32(%rsi)
	movq	%r11,%rcx
	movups	%xmm2,-16(%rsi)
	ja	.Lecb_dec_loop3

.Lecb_dec_tail:
	addq	$64,%rdx
	jz	.Lecb_ret

	cmpq	$16,%rdx
	movups	(%rdi),%xmm0
	je	.Lecb_dec_one
	cmpq	$32,%rdx
	movups	16(%rdi),%xmm1
	je	.Lecb_dec_two
	cmpq	$48,%rdx
	movups	32(%rdi),%xmm2
	je	.Lecb_dec_three
	movups	48(%rdi),%xmm3
	call	_aesni_decrypt4
	movups	%xmm0,(%rsi)
	movups	%xmm1,16(%rsi)
	movups	%xmm2,32(%rsi)
	movups	%xmm3,48(%rsi)
	jmp	.Lecb_ret
.align	16
.Lecb_dec_one:
	movaps	(%rcx),%xmm4
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
.Loop_dec1_4:
	aesdec	%xmm5,%xmm0
	decl	%eax
	movaps	(%rcx),%xmm5
	leaq	16(%rcx),%rcx
	jnz	.Loop_dec1_4	
	aesdeclast	%xmm5,%xmm0
	movups	%xmm0,(%rsi)
	jmp	.Lecb_ret
.align	16
.Lecb_dec_two:
	call	_aesni_decrypt3
	movups	%xmm0,(%rsi)
	movups	%xmm1,16(%rsi)
	jmp	.Lecb_ret
.align	16
.Lecb_dec_three:
	call	_aesni_decrypt3
	movups	%xmm0,(%rsi)
	movups	%xmm1,16(%rsi)
	movups	%xmm2,32(%rsi)

.Lecb_ret:
	repz
	retq
.size	aesni_ecb_encrypt,.-aesni_ecb_encrypt
.globl	aesni_cbc_encrypt
.type	aesni_cbc_encrypt,@function
.align	16
aesni_cbc_encrypt:
	testq	%rdx,%rdx
	jz	.Lcbc_ret

	movl	240(%rcx),%r10d
	movq	%rcx,%r11
	testl	%r9d,%r9d
	jz	.Lcbc_decrypt

	movups	(%r8),%xmm0
	cmpq	$16,%rdx
	movl	%r10d,%eax
	jb	.Lcbc_enc_tail
	subq	$16,%rdx
	jmp	.Lcbc_enc_loop
.align	16
.Lcbc_enc_loop:
	movups	(%rdi),%xmm1
	leaq	16(%rdi),%rdi
	pxor	%xmm1,%xmm0
	movaps	(%rcx),%xmm4
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
.Loop_enc1_5:
	aesenc	%xmm5,%xmm0
	decl	%eax
	movaps	(%rcx),%xmm5
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_5	
	aesenclast	%xmm5,%xmm0
	subq	$16,%rdx
	leaq	16(%rsi),%rsi
	movl	%r10d,%eax
	movq	%r11,%rcx
	movups	%xmm0,-16(%rsi)
	jnc	.Lcbc_enc_loop
	addq	$16,%rdx
	jnz	.Lcbc_enc_tail
	movups	%xmm0,(%r8)
	jmp	.Lcbc_ret

.Lcbc_enc_tail:
	movq	%rdx,%rcx
	xchgq	%rdi,%rsi
.long	0x9066A4F3	
	movl	$16,%ecx
	subq	%rdx,%rcx
	xorl	%eax,%eax
.long	0x9066AAF3	
	leaq	-16(%rdi),%rdi
	movl	%r10d,%eax
	movq	%rdi,%rsi
	movq	%r11,%rcx
	xorq	%rdx,%rdx
	jmp	.Lcbc_enc_loop	

.align	16
.Lcbc_decrypt:
	movups	(%r8),%xmm6
	subq	$64,%rdx
	movl	%r10d,%eax
	jbe	.Lcbc_dec_tail
	jmp	.Lcbc_dec_loop3
.align	16
.Lcbc_dec_loop3:
	movups	(%rdi),%xmm0
	movups	16(%rdi),%xmm1
	movups	32(%rdi),%xmm2
	movaps	%xmm0,%xmm7
	movaps	%xmm1,%xmm8
	movaps	%xmm2,%xmm9
	call	_aesni_decrypt3
	subq	$48,%rdx
	leaq	48(%rdi),%rdi
	leaq	48(%rsi),%rsi
	pxor	%xmm6,%xmm0
	pxor	%xmm7,%xmm1
	movaps	%xmm9,%xmm6
	pxor	%xmm8,%xmm2
	movups	%xmm0,-48(%rsi)
	movl	%r10d,%eax
	movups	%xmm1,-32(%rsi)
	movq	%r11,%rcx
	movups	%xmm2,-16(%rsi)
	ja	.Lcbc_dec_loop3

.Lcbc_dec_tail:
	addq	$64,%rdx
	movups	%xmm6,(%r8)
	jz	.Lcbc_dec_ret

	movups	(%rdi),%xmm0
	cmpq	$16,%rdx
	movaps	%xmm0,%xmm7
	jbe	.Lcbc_dec_one
	movups	16(%rdi),%xmm1
	cmpq	$32,%rdx
	movaps	%xmm1,%xmm8
	jbe	.Lcbc_dec_two
	movups	32(%rdi),%xmm2
	cmpq	$48,%rdx
	movaps	%xmm2,%xmm9
	jbe	.Lcbc_dec_three
	movups	48(%rdi),%xmm3
	call	_aesni_decrypt4
	pxor	%xmm6,%xmm0
	movups	48(%rdi),%xmm6
	pxor	%xmm7,%xmm1
	movups	%xmm0,(%rsi)
	pxor	%xmm8,%xmm2
	movups	%xmm1,16(%rsi)
	pxor	%xmm9,%xmm3
	movups	%xmm2,32(%rsi)
	movaps	%xmm3,%xmm0
	leaq	48(%rsi),%rsi
	jmp	.Lcbc_dec_tail_collected
.align	16
.Lcbc_dec_one:
	movaps	(%rcx),%xmm4
	movaps	16(%rcx),%xmm5
	leaq	32(%rcx),%rcx
	pxor	%xmm4,%xmm0
.Loop_dec1_6:
	aesdec	%xmm5,%xmm0
	decl	%eax
	movaps	(%rcx),%xmm5
	leaq	16(%rcx),%rcx
	jnz	.Loop_dec1_6	
	aesdeclast	%xmm5,%xmm0
	pxor	%xmm6,%xmm0
	movaps	%xmm7,%xmm6
	jmp	.Lcbc_dec_tail_collected
.align	16
.Lcbc_dec_two:
	call	_aesni_decrypt3
	pxor	%xmm6,%xmm0
	pxor	%xmm7,%xmm1
	movups	%xmm0,(%rsi)
	movaps	%xmm8,%xmm6
	movaps	%xmm1,%xmm0
	leaq	16(%rsi),%rsi
	jmp	.Lcbc_dec_tail_collected
.align	16
.Lcbc_dec_three:
	call	_aesni_decrypt3
	pxor	%xmm6,%xmm0
	pxor	%xmm7,%xmm1
	movups	%xmm0,(%rsi)
	pxor	%xmm8,%xmm2
	movups	%xmm1,16(%rsi)
	movaps	%xmm9,%xmm6
	movaps	%xmm2,%xmm0
	leaq	32(%rsi),%rsi
	jmp	.Lcbc_dec_tail_collected
.align	16
.Lcbc_dec_tail_collected:
	andq	$15,%rdx
	movups	%xmm6,(%r8)
	jnz	.Lcbc_dec_tail_partial
	movups	%xmm0,(%rsi)
	jmp	.Lcbc_dec_ret
.Lcbc_dec_tail_partial:
	movaps	%xmm0,-24(%rsp)
	movq	%rsi,%rdi
	movq	%rdx,%rcx
	leaq	-24(%rsp),%rsi
.long	0x9066A4F3	

.Lcbc_dec_ret:
.Lcbc_ret:
	repz
	retq
.size	aesni_cbc_encrypt,.-aesni_cbc_encrypt
.globl	aesni_set_decrypt_key
.type	aesni_set_decrypt_key,@function
.align	16
aesni_set_decrypt_key:
	subq	$8,%rsp
	call	_aesni_set_encrypt_key
	shll	$4,%esi
	testl	%eax,%eax
	jnz	.Ldec_key_ret
	leaq	16(%rdx,%rsi,1),%rdi

	movaps	(%rdx),%xmm0
	movaps	(%rdi),%xmm1
	movaps	%xmm0,(%rdi)
	movaps	%xmm1,(%rdx)
	leaq	16(%rdx),%rdx
	leaq	-16(%rdi),%rdi

.Ldec_key_inverse:
	movaps	(%rdx),%xmm0
	movaps	(%rdi),%xmm1
	aesimc	%xmm0,%xmm0
	aesimc	%xmm1,%xmm1
	leaq	16(%rdx),%rdx
	leaq	-16(%rdi),%rdi
	cmpq	%rdx,%rdi
	movaps	%xmm0,16(%rdi)
	movaps	%xmm1,-16(%rdx)
	ja	.Ldec_key_inverse

	movaps	(%rdx),%xmm0
	aesimc	%xmm0,%xmm0
	movaps	%xmm0,(%rdi)
.Ldec_key_ret:
	addq	$8,%rsp
	repz
	retq
.LSEH_end_set_decrypt_key:
.size	aesni_set_decrypt_key,.-aesni_set_decrypt_key
.globl	aesni_set_encrypt_key
.type	aesni_set_encrypt_key,@function
.align	16
aesni_set_encrypt_key:
_aesni_set_encrypt_key:
	subq	$8,%rsp
	testq	%rdi,%rdi
	movq	$-1,%rax
	jz	.Lenc_key_ret
	testq	%rdx,%rdx
	jz	.Lenc_key_ret

	movups	(%rdi),%xmm0
	pxor	%xmm4,%xmm4
	leaq	16(%rdx),%rax
	cmpl	$256,%esi
	je	.L14rounds
	cmpl	$192,%esi
	je	.L12rounds
	cmpl	$128,%esi
	jne	.Lbad_keybits

.L10rounds:
	movl	$9,%esi
	movaps	%xmm0,(%rdx)
	aeskeygenassist	$1,%xmm0,%xmm1
	call	.Lkey_expansion_128_cold
	aeskeygenassist	$2,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$4,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$8,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$16,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$32,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$64,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$128,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$27,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$54,%xmm0,%xmm1
	call	.Lkey_expansion_128
	movaps	%xmm0,(%rax)
	movl	%esi,80(%rax)
	xorl	%eax,%eax
	jmp	.Lenc_key_ret

.align	16
.L12rounds:
	movq	16(%rdi),%xmm2
	movl	$11,%esi
	movaps	%xmm0,(%rdx)
	aeskeygenassist	$1,%xmm2,%xmm1
	call	.Lkey_expansion_192a_cold
	aeskeygenassist	$2,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	aeskeygenassist	$4,%xmm2,%xmm1
	call	.Lkey_expansion_192a
	aeskeygenassist	$8,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	aeskeygenassist	$16,%xmm2,%xmm1
	call	.Lkey_expansion_192a
	aeskeygenassist	$32,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	aeskeygenassist	$64,%xmm2,%xmm1
	call	.Lkey_expansion_192a
	aeskeygenassist	$128,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	movaps	%xmm0,(%rax)
	movl	%esi,48(%rax)
	xorq	%rax,%rax
	jmp	.Lenc_key_ret

.align	16
.L14rounds:
	movups	16(%rdi),%xmm2
	movl	$13,%esi
	leaq	16(%rax),%rax
	movaps	%xmm0,(%rdx)
	movaps	%xmm2,16(%rdx)
	aeskeygenassist	$1,%xmm2,%xmm1
	call	.Lkey_expansion_256a_cold
	aeskeygenassist	$1,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$2,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$2,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$4,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$4,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$8,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$8,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$16,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$16,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$32,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$32,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$64,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	movaps	%xmm0,(%rax)
	movl	%esi,16(%rax)
	xorq	%rax,%rax
	jmp	.Lenc_key_ret

.align	16
.Lbad_keybits:
	movq	$-2,%rax
.Lenc_key_ret:
	addq	$8,%rsp
	repz
	retq
.LSEH_end_set_encrypt_key:

.align	16
.Lkey_expansion_128:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax
.Lkey_expansion_128_cold:
	shufps	$16,%xmm0,%xmm4
	pxor	%xmm4,%xmm0
	shufps	$140,%xmm0,%xmm4
	pxor	%xmm4,%xmm0
	pshufd	$255,%xmm1,%xmm1
	pxor	%xmm1,%xmm0
	repz
	retq

.align	16
.Lkey_expansion_192a:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax
.Lkey_expansion_192a_cold:
	movaps	%xmm2,%xmm5
.Lkey_expansion_192b_warm:
	shufps	$16,%xmm0,%xmm4
	movaps	%xmm2,%xmm3
	pxor	%xmm4,%xmm0
	shufps	$140,%xmm0,%xmm4
	pslldq	$4,%xmm3
	pxor	%xmm4,%xmm0
	pshufd	$85,%xmm1,%xmm1
	pxor	%xmm3,%xmm2
	pxor	%xmm1,%xmm0
	pshufd	$255,%xmm0,%xmm3
	pxor	%xmm3,%xmm2
	repz
	retq

.align	16
.Lkey_expansion_192b:
	movaps	%xmm0,%xmm3
	shufps	$68,%xmm0,%xmm5
	movaps	%xmm5,(%rax)
	shufps	$78,%xmm2,%xmm3
	movaps	%xmm3,16(%rax)
	leaq	32(%rax),%rax
	jmp	.Lkey_expansion_192b_warm

.align	16
.Lkey_expansion_256a:
	movaps	%xmm2,(%rax)
	leaq	16(%rax),%rax
.Lkey_expansion_256a_cold:
	shufps	$16,%xmm0,%xmm4
	pxor	%xmm4,%xmm0
	shufps	$140,%xmm0,%xmm4
	pxor	%xmm4,%xmm0
	pshufd	$255,%xmm1,%xmm1
	pxor	%xmm1,%xmm0
	repz
	retq

.align	16
.Lkey_expansion_256b:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax

	shufps	$16,%xmm2,%xmm4
	pxor	%xmm4,%xmm2
	shufps	$140,%xmm2,%xmm4
	pxor	%xmm4,%xmm2
	pshufd	$170,%xmm1,%xmm1
	pxor	%xmm1,%xmm2
	repz
	retq
.size	aesni_set_encrypt_key,.-aesni_set_encrypt_key
