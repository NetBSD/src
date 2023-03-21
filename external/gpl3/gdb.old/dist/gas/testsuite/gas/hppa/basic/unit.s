	.level 1.1
	.code
	.align 4
; Basic unit instruction tests.  
;
; We could/should test some of the corner cases for register and 
; immediate fields.  We should also check the assorted field
; selectors to make sure they're handled correctly.

	uxor %r4,%r5,%r6
	uxor,sbz %r4,%r5,%r6
	uxor,shz %r4,%r5,%r6
	uxor,tr %r4,%r5,%r6
	uxor,nbz %r4,%r5,%r6
	uxor,nhz %r4,%r5,%r6

	uaddcm %r4,%r5,%r6
	uaddcm,sbz %r4,%r5,%r6
	uaddcm,shz %r4,%r5,%r6
	uaddcm,sdc %r4,%r5,%r6
	uaddcm,sbc %r4,%r5,%r6
	uaddcm,shc %r4,%r5,%r6
	uaddcm,tr %r4,%r5,%r6
	uaddcm,nbz %r4,%r5,%r6
	uaddcm,nhz %r4,%r5,%r6
	uaddcm,ndc %r4,%r5,%r6
	uaddcm,nbc %r4,%r5,%r6
	uaddcm,nhc %r4,%r5,%r6

	uaddcmt %r4,%r5,%r6
	uaddcmt,sbz %r4,%r5,%r6
	uaddcmt,shz %r4,%r5,%r6
	uaddcmt,sdc %r4,%r5,%r6
	uaddcmt,sbc %r4,%r5,%r6
	uaddcmt,shc %r4,%r5,%r6
	uaddcmt,tr %r4,%r5,%r6
	uaddcmt,nbz %r4,%r5,%r6
	uaddcmt,nhz %r4,%r5,%r6
	uaddcmt,ndc %r4,%r5,%r6
	uaddcmt,nbc %r4,%r5,%r6
	uaddcmt,nhc %r4,%r5,%r6

	uaddcm,tc %r4,%r5,%r6
	uaddcm,tc,sbz %r4,%r5,%r6
	uaddcm,tc,shz %r4,%r5,%r6
	uaddcm,tc,sdc %r4,%r5,%r6
	uaddcm,tc,sbc %r4,%r5,%r6
	uaddcm,tc,shc %r4,%r5,%r6
	uaddcm,tc,tr %r4,%r5,%r6
	uaddcm,tc,nbz %r4,%r5,%r6
	uaddcm,tc,nhz %r4,%r5,%r6
	uaddcm,tc,ndc %r4,%r5,%r6
	uaddcm,tc,nbc %r4,%r5,%r6
	uaddcm,tc,nhc %r4,%r5,%r6
