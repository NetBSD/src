	.text

#; func_locvars
#; - function with a space on the stack 
#;   allocated for local variables

func_locvars:
	.cfi_startproc
	
	#; alocate space for local vars
	sub	$0x1234,%rsp
	.cfi_adjust_cfa_offset	0x1234
	
	#; dummy body
	movl	$1,%eax
	
	#; release space of local vars and return
	add	$0x1234,%rsp
	.cfi_adjust_cfa_offset	-0x1234
	ret
	.cfi_endproc

#; func_prologue
#; - functions that begins with standard
#;   prologue: "pushq %rbp; movq %rsp,%rbp"

func_prologue:
	.cfi_startproc
	
	#; prologue, CFI is valid after 
	#; each instruction.
	pushq	%rbp
	.cfi_def_cfa_offset	16
	.cfi_offset		%rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register	%rbp

	#; function body
	call	func_locvars
	addl	$3, %eax

	#; epilogue with valid CFI
	#; (we're better than gcc :-)
	leaveq
	.cfi_def_cfa		%rsp, 8
	ret
	.cfi_endproc

#; func_otherreg
#; - function that moves frame pointer to 
#;   another register (r12) and then allocates 
#;   a space for local variables

func_otherreg:
	.cfi_startproc

	#; save frame pointer to r8
	movq	%rsp,%r8
	.cfi_def_cfa_register	r8

	#; alocate space for local vars
	#;  (no .cfi_{def,adjust}_cfa_offset here,
	#;   because CFA is computed from r8!)
	sub	$100,%rsp

	#; function body
	call	func_prologue
	addl	$2, %eax
	
	#; restore frame pointer from r8
	movq	%r8,%rsp
	.cfi_def_cfa_register	rsp
	ret
	.cfi_endproc

#; main
#; - typical function
main:
	.cfi_startproc
	
	#; only function body that doesn't
	#; touch the stack at all.
	call	func_otherreg
	
	#; return
	ret
	.cfi_endproc

#; _start
#; - standard entry point

	.globl	_start
_start:
	.cfi_startproc
	call	main
	movq	%rax,%rdi
	movq	$0x3c,%rax
	syscall
	hlt
	.cfi_endproc

#; func_alldirectives
#; - test for all .cfi directives. 
#;   This function is never called and the CFI info doesn't make sense.

func_alldirectives:
	.cfi_startproc simple
	.cfi_def_cfa	rsp,8
	nop
	.cfi_def_cfa_offset	16
	nop
	.cfi_def_cfa_register	r8
	nop
	.cfi_adjust_cfa_offset	0x1234
	nop
	.cfi_offset	%rsi, 0x10
	nop
	.cfi_register	%r8, %r9
	nop
	.cfi_remember_state
	nop
	.cfi_restore %rbp
	nop
	.cfi_undefined %rip
	nop
	.cfi_same_value rbx
	nop
	.cfi_restore_state
	ret
	.cfi_endproc

#; func_all_registers
#; - test for all .cfi register numbers. 
#;   This function is never called and the CFI info doesn't make sense.

func_all_registers:
	.cfi_startproc simple

	.cfi_undefined rip	; nop
	.cfi_undefined rax	; nop
	.cfi_undefined rcx	; nop
	.cfi_undefined rdx	; nop
	.cfi_undefined rbx	; nop
	.cfi_undefined rsp	; nop
	.cfi_undefined rbp	; nop
	.cfi_undefined rsi	; nop
	.cfi_undefined rdi	; nop
	.cfi_undefined r8	; nop
	.cfi_undefined r9	; nop
	.cfi_undefined r10	; nop
	.cfi_undefined r11	; nop
	.cfi_undefined r12	; nop
	.cfi_undefined r13	; nop
	.cfi_undefined r14	; nop
	.cfi_undefined r15	; nop
	.cfi_undefined rflags	; nop

	.cfi_undefined es	; nop
	.cfi_undefined cs	; nop
	.cfi_undefined ds	; nop
	.cfi_undefined ss	; nop
	.cfi_undefined fs	; nop
	.cfi_undefined gs	; nop
	.cfi_undefined tr	; nop
	.cfi_undefined ldtr	; nop
	.cfi_undefined fs.base	; nop
	.cfi_undefined gs.base	; nop

	.cfi_undefined mxcsr	; nop
	.cfi_undefined xmm0	; nop
	.cfi_undefined xmm1	; nop
	.cfi_undefined xmm2	; nop
	.cfi_undefined xmm3	; nop
	.cfi_undefined xmm4	; nop
	.cfi_undefined xmm5	; nop
	.cfi_undefined xmm6	; nop
	.cfi_undefined xmm7	; nop
	.cfi_undefined xmm8	; nop
	.cfi_undefined xmm9	; nop
	.cfi_undefined xmm10	; nop
	.cfi_undefined xmm11	; nop
	.cfi_undefined xmm12	; nop
	.cfi_undefined xmm13	; nop
	.cfi_undefined xmm14	; nop
	.cfi_undefined xmm15	; nop

	.cfi_undefined fcw	; nop
	.cfi_undefined fsw	; nop
	.cfi_undefined st	; nop
	.cfi_undefined st(1)	; nop
	.cfi_undefined st(2)	; nop
	.cfi_undefined st(3)	; nop
	.cfi_undefined st(4)	; nop
	.cfi_undefined st(5)	; nop
	.cfi_undefined st(6)	; nop
	.cfi_undefined st(7)	; nop

	.cfi_undefined mm0	; nop
	.cfi_undefined mm1	; nop
	.cfi_undefined mm2	; nop
	.cfi_undefined mm3	; nop
	.cfi_undefined mm4	; nop
	.cfi_undefined mm5	; nop
	.cfi_undefined mm6	; nop
	.cfi_undefined mm7	; nop

	.cfi_endproc
