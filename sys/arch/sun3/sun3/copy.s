
.text
/*
 * copyinstr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 * NOTE: maxlength must be < 64K
 */
ENTRY(copyinstr)
	movl	_curpcb,a0		| current pcb
	movl	#Lcisflt1,a0@(PCB_ONFAULT) | set up to catch faults
	movl	sp@(4),a0		| a0 = fromaddr
	movl	sp@(8),a1		| a1 = toaddr
	moveq	#0,d0
	movw	sp@(14),d0		| d0 = maxlength
	jlt	Lcisflt1		| negative count, error
	jeq	Lcisdone		| zero count, all done
	subql	#1,d0			| set up for dbeq
Lcisloop:
	movsb	a0@+,d1			| grab a byte
	movb	d1,a1@+			| copy it
	dbeq	d0,Lcisloop		| if !null and more, continue
	jne	Lcisflt2		| ran out of room, error
	moveq	#0,d0			| got a null, all done
Lcisdone:
	tstl	sp@(16)			| return length desired?
	jeq	Lcisret			| no, just return
	subl	sp@(4),a0		| determine how much was copied
	movl	sp@(16),a1		| return location
	movl	a0,a1@			| stash it
Lcisret:
	movl	_curpcb,a0		| current pcb
	clrl	a0@(PCB_ONFAULT) 	| clear fault addr
	rts
Lcisflt1:
	moveq	#EFAULT,d0		| copy fault
	jra	Lcisdone
Lcisflt2:
	moveq	#ENAMETOOLONG,d0	| ran out of space
	jra	Lcisdone	

/*
 * copyoutstr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 * NOTE: maxlength must be < 64K
 */
ENTRY(copyoutstr)
	movl	_curpcb,a0		| current pcb
	movl	#Lcosflt1,a0@(PCB_ONFAULT) | set up to catch faults
	movl	sp@(4),a0		| a0 = fromaddr
	movl	sp@(8),a1		| a1 = toaddr
	moveq	#0,d0
	movw	sp@(14),d0		| d0 = maxlength
	jlt	Lcosflt1		| negative count, error
	jeq	Lcosdone		| zero count, all done
	subql	#1,d0			| set up for dbeq
Lcosloop:
	movb	a0@+,d1			| grab a byte
	movsb	d1,a1@+			| copy it
	dbeq	d0,Lcosloop		| if !null and more, continue
	jne	Lcosflt2		| ran out of room, error
	moveq	#0,d0			| got a null, all done
Lcosdone:
	tstl	sp@(16)			| return length desired?
	jeq	Lcosret			| no, just return
	subl	sp@(4),a0		| determine how much was copied
	movl	sp@(16),a1		| return location
	movl	a0,a1@			| stash it
Lcosret:
	movl	_curpcb,a0		| current pcb
	clrl	a0@(PCB_ONFAULT) 	| clear fault addr
	rts
Lcosflt1:
	moveq	#EFAULT,d0		| copy fault
	jra	Lcosdone
Lcosflt2:
	moveq	#ENAMETOOLONG,d0	| ran out of space
	jra	Lcosdone	

/*
 * copystr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from one point to another in
 * the kernel address space.
 * NOTE: maxlength must be < 64K
 */
ENTRY(copystr)
	movl	sp@(4),a0		| a0 = fromaddr
	movl	sp@(8),a1		| a1 = toaddr
	moveq	#0,d0
	movw	sp@(14),d0		| d0 = maxlength
	jlt	Lcsflt1			| negative count, error
	jeq	Lcsdone			| zero count, all done
	subql	#1,d0			| set up for dbeq
Lcsloop:
	movb	a0@+,a1@+		| copy a byte
	dbeq	d0,Lcsloop		| if !null and more, continue
	jne	Lcsflt2			| ran out of room, error
	moveq	#0,d0			| got a null, all done
Lcsdone:
	tstl	sp@(16)			| return length desired?
	jeq	Lcsret			| no, just return
	subl	sp@(4),a0		| determine how much was copied
	movl	sp@(16),a1		| return location
	movl	a0,a1@			| stash it
Lcsret:
	rts
Lcsflt1:
	moveq	#EFAULT,d0		| copy fault
	jra	Lcsdone
Lcsflt2:
	moveq	#ENAMETOOLONG,d0	| ran out of space
	jra	Lcsdone	

/* 
 * Copyin(from, to, len)
 *
 * Copy specified amount of data from user space into the kernel.
 * NOTE: len must be < 64K
 */
ENTRY(copyin)
	movl	d2,sp@-			| scratch register
	movl	_curpcb,a0		| current pcb
	movl	#Lciflt,a0@(PCB_ONFAULT) | set up to catch faults
	movl	sp@(16),d2		| check count
	jlt	Lciflt			| negative, error
	jeq	Lcidone			| zero, done
	movl	sp@(8),a0		| src address
	movl	sp@(12),a1		| dest address
	movl	a0,d0
	btst	#0,d0			| src address odd?
	jeq	Lcieven			| no, go check dest
	movsb	a0@+,d1			| yes, get a byte
	movb	d1,a1@+			| put a byte
	subql	#1,d2			| adjust count
	jeq	Lcidone			| exit if done
Lcieven:
	movl	a1,d0
	btst	#0,d0			| dest address odd?
	jne	Lcibyte			| yes, must copy by bytes
	movl	d2,d0			| no, get count
	lsrl	#2,d0			| convert to longwords
	jeq	Lcibyte			| no longwords, copy bytes
	subql	#1,d0			| set up for dbf
Lcilloop:
	movsl	a0@+,d1			| get a long
	movl	d1,a1@+			| put a long
	dbf	d0,Lcilloop		| til done
	andl	#3,d2			| what remains
	jeq	Lcidone			| all done
Lcibyte:
	subql	#1,d2			| set up for dbf
Lcibloop:
	movsb	a0@+,d1			| get a byte
	movb	d1,a1@+			| put a byte
	dbf	d2,Lcibloop		| til done
Lcidone:
	moveq	#0,d0			| success
Lciexit:
	movl	_curpcb,a0		| current pcb
	clrl	a0@(PCB_ONFAULT) 	| clear fault catcher
	movl	sp@+,d2			| restore scratch reg
	rts
Lciflt:
	moveq	#EFAULT,d0		| got a fault
	jra	Lciexit

/* 
 * Copyout(from, to, len)
 *
 * Copy specified amount of data from kernel to the user space
 * NOTE: len must be < 64K
 */
ENTRY(copyout)
	movl	d2,sp@-			| scratch register
	movl	_curpcb,a0		| current pcb
	movl	#Lcoflt,a0@(PCB_ONFAULT) | catch faults
	movl	sp@(16),d2		| check count
	jlt	Lcoflt			| negative, error
	jeq	Lcodone			| zero, done
	movl	sp@(8),a0		| src address
	movl	sp@(12),a1		| dest address
	movl	a0,d0
	btst	#0,d0			| src address odd?
	jeq	Lcoeven			| no, go check dest
	movb	a0@+,d1			| yes, get a byte
	movsb	d1,a1@+			| put a byte
	subql	#1,d2			| adjust count
	jeq	Lcodone			| exit if done
Lcoeven:
	movl	a1,d0
	btst	#0,d0			| dest address odd?
	jne	Lcobyte			| yes, must copy by bytes
	movl	d2,d0			| no, get count
	lsrl	#2,d0			| convert to longwords
	jeq	Lcobyte			| no longwords, copy bytes
	subql	#1,d0			| set up for dbf
Lcolloop:
	movl	a0@+,d1			| get a long
	movsl	d1,a1@+			| put a long
	dbf	d0,Lcolloop		| til done
	andl	#3,d2			| what remains
	jeq	Lcodone			| all done
Lcobyte:
	subql	#1,d2			| set up for dbf
Lcobloop:
	movb	a0@+,d1			| get a byte
	movsb	d1,a1@+			| put a byte
	dbf	d2,Lcobloop		| til done
Lcodone:
	moveq	#0,d0			| success
Lcoexit:
	movl	_curpcb,a0		| current pcb
	clrl	a0@(PCB_ONFAULT) 	| clear fault catcher
	movl	sp@+,d2			| restore scratch reg
	rts
Lcoflt:
	moveq	#EFAULT,d0		| got a fault
	jra	Lcoexit

/*
 * {fu,su},{byte,sword,word}
 */
ALTENTRY(fuiword, _fuword)
ENTRY(fuword)
	movl	sp@(4),a0		| address to read
	movl	_curpcb,a1		| current pcb
	movl	#Lfserr,a1@(PCB_ONFAULT) | where to return to on a fault
	movsl	a0@,d0			| do read from user space
	jra	Lfsdone

ENTRY(fusword)
	movl	sp@(4),a0
	movl	_curpcb,a1		| current pcb
	movl	#Lfserr,a1@(PCB_ONFAULT) | where to return to on a fault
	moveq	#0,d0
	movsw	a0@,d0			| do read from user space
	jra	Lfsdone

ALTENTRY(fuibyte, _fubyte)
ENTRY(fubyte)
	movl	sp@(4),a0		| address to read
	movl	_curpcb,a1		| current pcb
	movl	#Lfserr,a1@(PCB_ONFAULT) | where to return to on a fault
	moveq	#0,d0
	movsb	a0@,d0			| do read from user space
	jra	Lfsdone

Lfserr:
	moveq	#-1,d0			| error indicator
Lfsdone:
	clrl	a1@(PCB_ONFAULT) 	| clear fault address
	rts

ALTENTRY(suiword, _suword)
ENTRY(suword)
	movl	sp@(4),a0		| address to write
	movl	sp@(8),d0		| value to put there
	movl	_curpcb,a1		| current pcb
	movl	#Lfserr,a1@(PCB_ONFAULT) | where to return to on a fault
	movsl	d0,a0@			| do write to user space
	moveq	#0,d0			| indicate no fault
	jra	Lfsdone

ENTRY(susword)
	movl	sp@(4),a0		| address to write
	movw	sp@(10),d0		| value to put there
	movl	_curpcb,a1		| current pcb
	movl	#Lfserr,a1@(PCB_ONFAULT) | where to return to on a fault
	movsw	d0,a0@			| do write to user space
	moveq	#0,d0			| indicate no fault
	jra	Lfsdone

ALTENTRY(suibyte, _subyte)
ENTRY(subyte)
	movl	sp@(4),a0		| address to write
	movb	sp@(11),d0		| value to put there
	movl	_curpcb,a1		| current pcb
	movl	#Lfserr,a1@(PCB_ONFAULT) | where to return to on a fault
	movsb	d0,a0@			| do write to user space
	moveq	#0,d0			| indicate no fault
	jra	Lfsdone
