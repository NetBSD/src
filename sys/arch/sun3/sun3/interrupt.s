.globl _cnt

.data
.globl _intrcnt
_intrcnt:
	/*    0  1  2  3  4  5  6  7, spurious */
	.long 0, 0, 0, 0, 0, 0, 0, 0, 0

.text

#define INTERRUPT_HANDLE(interrupt num) \
	addql #1,_intrcnt+interrupt_num	\
	movw	sr,sp@-			| push current SR value \
	clrw	sp@-			|    padded to longword \
	jbsr	_intrhand		| handle interrupt 	\
	addql	#4,sp			| pop SR
	moveml	sp@+,#0x0303					\
	addql	#2,sp						\
	addql #1, _cnt+V_INTR					\
	jra rei

.globl _level1intr, _level2intr, _level3intr, _level4intr
.globl _level5intr, _level6intr, _level7intr

/* system enable register 1 */
_level1intr:	
	INTERRUPT_HANDLE(1)
/* system enable register 2, SCSI */
_level2intr: 
	INTERRUPT_HANDLE(2)

/* system enable register 3, Ethernet */
_level3intr:
	INTERRUPT_HANDLE(3)

/* video */
_level4intr:
	INTERRUPT_HANDLE(4)

/* clock */
_level5intr:
	INTERRUPT_HANDLE(5)

/* SCCs */
_level6intr:
	INTERRUPT_HANDLE(6)

/* Memory Error */
_level7intr:
	INTERRUPT_HANDLE(7)

_spurintr:
	
