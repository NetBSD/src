/* define memory list passed by loadbsd 
 *
 *	$Id: memlist.h,v 1.2 1994/02/11 06:59:50 chopps Exp $
 */

struct Mem_List {
	u_int	num_mem;
	struct Mem_Seg {
		u_int	mem_start;
		u_int	mem_size;
		u_short	mem_attrib;
		short	mem_pri;
	} mem_seg[1];
};

/* some attribute flags we are interested in */
#define MEMF_CHIP	(1L<<1)	/* Chip memory */
#define MEMF_FAST	(1L<<2)	/* Fast memory */
#define MEMF_24BITDMA	(1L<<9)	/* DMAable memory in 68K address space */
