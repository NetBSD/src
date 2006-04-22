/*	$NetBSD: stackframe.h,v 1.1.6.2 2006/04/22 11:37:39 simonb Exp $	*/

/* 
 * Contributed to the NetBSD foundation by Cherry G. Mathew
 */

#define UNW_VER(x)           ((x) >> 48)
#define UNW_FLAG_MASK        0x0000ffff00000000L
#define UNW_FLAG_OSMASK      0x0000f00000000000L
#define UNW_FLAG_EHANDLER(x) ((x) & 0x0000000100000000L)
#define UNW_FLAG_UHANDLER(x) ((x) & 0x0000000200000000L)
#define UNW_LENGTH(x)        ((x) & 0x00000000ffffffffL)

/* Unwind table entry. */
struct uwtable_ent {
	uint64_t start;
	uint64_t end;
	char *infoptr;
};


enum regrecord_type{
	UNSAVED,	/* Register contents live ( and therefore untouched ). */
	IMMED,		/* .offset field is the saved content. */
	BRREL,		/* Register saved in one of the Branch Registers. */
	GRREL,		/* Register saved in one of the Stacked GRs 
			 * regstate.offset contains GR number (usually >= 32)
			 */
	SPREL,		/* Register saved on the memory stack frame. 
			 * regstate.offset is in words; ie; location == (sp + 4 * spoff). 
			 */
	PSPREL		/* Register saved on the memory stack frame but offseted via psp 
			 * regstate.offset is in words; ie; location == (psp + 16 â€“ 4 * pspoff) 
			 */
}; 


struct regstate {
	enum regrecord_type where;
	uint64_t when;	

#define INVALID -1UL	/* Indicates uninitialised offset value. */
	uint64_t offset;
};


/* A staterecord contains the net state of 
 * sequentially parsing unwind descriptors.
 * The entry state of the current prologue region
 * is the exit state of the previous region.
 * We record info about registers we care about
 * ie; just enough to re-construct an unwind frame,
 * and ignore the rest.
 * Initial state is where = UNSAVED for all .where fields.
 */

struct staterecord {
	struct regstate bsp;
   	struct regstate psp;
 	struct regstate rp;
 	struct regstate pfs;
};

/* The unwind frame is a simpler version of the trap frame
 * and contains a subset of preserved registers, which are 
 * usefull in unwinding an ia64 stack frame.
 * Keep this in sync with the staterecord. See: stackframe.c:updateregs()
 */
   
struct unwind_frame {
	uint64_t		bsp;	/* Base of the RSE. !!! XXX: Stack Frame discontinuities */
	uint64_t		psp;	/* Mem stack (variable size) base. */
	uint64_t		rp;	/* Return Pointer */
	uint64_t		pfs;	/* Previous Frame size info */

	/* Don't mirror anything below this line with struct staterecord */
	uint64_t		sp;
};


void buildrecordchain(uint64_t, struct recordchain *);
void initrecord(struct staterecord *);
void modifyrecord(struct staterecord *, struct recordchain *, uint64_t);
void pushrecord(struct staterecord *);
void poprecord(struct staterecord *, int);
void dump_staterecord(struct staterecord *);
void clonerecordstack(u_int);
void switchrecordstack(u_int);

struct uwtable_ent *
get_unwind_table_entry(uint64_t);
void 
patchunwindframe(struct unwind_frame *, uint64_t, uint64_t);
void
updateregs(struct unwind_frame *uwf, struct staterecord *, uint64_t) ;
struct uwtable_ent * get_unwind_table_entry(uint64_t ip);

struct staterecord *
buildrecordstack(struct recordchain *, uint64_t);
void dump_recordchain(struct recordchain *);

/* Convenience macros to decompose CFM & ar.pfs. */
#define	IA64_CFM_SOF(x)		((x) & 0x7f)
#define	IA64_CFM_SOL(x)		(((x) >> 7) & 0x7f)
#define	IA64_CFM_SOR(x)		(((x) >> 14) & 0x0f)
#define	IA64_CFM_RRB_GR(x)	(((x) >> 18) & 0x7f)
#define	IA64_CFM_RRB_FR(x)	(((x) >> 25) & 0x7f)
#define	IA64_CFM_RRB_PR(x)	(((x) >> 32) & 0x3f)

#define IA64_RNATINDEX(x)	(((x) & 0x1f8) >> 3)

/* Obeys Table 6:2 RSE Operation Instructions and State Modification */

/* These functions adjust for RSE rnat saves to bsp in the forward and 
 * reverse directions respectively.
 */
#define ia64_rnat_adjust ia64_bsp_adjust_call

static __inline uint64_t
ia64_bsp_adjust_call(uint64_t bsp, int sol)
{
	bsp += ((sol + (IA64_RNATINDEX(bsp) + sol) / 63) << 3);
	return bsp;
}

static __inline uint64_t
ia64_bsp_adjust_ret(uint64_t bsp, int sol)
{
	bsp -= ((sol + (62 - IA64_RNATINDEX(bsp) + sol) / 63) << 3);
	return bsp;
}

static __inline uint64_t
ia64_getrse_gr(uint64_t bsp, uint64_t gr)
{
	bsp = ia64_bsp_adjust_call(bsp, gr);
	return *(uint64_t *) bsp;
}
