
ENTRY(getvbr)
	movc vbr, d0
	rts

ENTRY(setvbr)
	movl sp@(4), d0
	movc d0, vbr
	rts

/* void control_copy_byte(caddr_t from, caddr_t to, int size)*/

ENTRY(control_copy_byte)
		
	movl sp@(4), a0			|a0 = from
	movl sp@(8), a1			|a1 = to 
	movl sp@(12), d1		|d1 = size	
	movl d2, sp@-			| save reg so we can use it for temp
	movc sfc, d0			| save sfc
	movl #FC_CONTROL, d2
	movc d2, sfc
	subqw #1, d1

loop:   movsb a0@+, d2
	movb  d2, a1@+
	dbra d1, loop

	movc d0, sfc
	movl sp@+, d2
	rts
	
	
	
