/*	$NetBSD: stackframe.c,v 1.5.6.1 2016/10/05 20:55:30 skrll Exp $	*/

/* Contributed to the NetBSD foundation by Cherry G. Mathew <cherry@mahiti.org>
 * This file contains routines to use decoded unwind descriptor entries
 * to build a stack configuration. The unwinder consults the stack 
 * configuration to fetch registers used to unwind the frame.
 * References:
 *	[1] section. 11.4.2.6., Itanium Software Conventions and 
 *			Runtime Architecture Guide.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>


#include <ia64/unwind/decode.h>
#include <ia64/unwind/stackframe.h>

//#define UNWIND_DIAGNOSTIC

/*
 * Global variables:
 * array of struct recordchain
 * size of record chain array.
 */
struct recordchain strc[MAXSTATERECS];
int rec_cnt = 0;

/*
 * Build a recordchain of a region, given the pointer to unwind table
 * entry, and the number of entries to decode.
 */
void
buildrecordchain(uint64_t unwind_infop, struct recordchain *xxx)
{
	uint64_t unwindstart, unwindend;
	uint64_t unwindlen;
	uint64_t region_len = 0;
	bool region_type = false; /* Prologue */

	struct unwind_hdr_t {
		uint64_t uwh;
	} *uwhp = (void *) unwind_infop;

	char *nextrecp, *recptr = (char *) unwind_infop + sizeof(uint64_t);
	
	unwindstart = (uint64_t) recptr;

	if (UNW_VER(uwhp->uwh) != 1) {
		printf("Wrong unwind version! \n");
		return;
	}
	
	unwindlen = UNW_LENGTH(uwhp->uwh) * sizeof(uint64_t);
	unwindend = unwindstart + unwindlen;

#ifdef UNWIND_DIAGNOSTIC
	printf("recptr = %p \n", recptr);
	printf("unwindlen = %lx \n", unwindlen);
	printf("unwindend = %lx \n", unwindend);
#endif

	/* XXX: Ignore zero length records. */


	for (rec_cnt = 0;
	    rec_cnt < MAXSTATERECS && (uint64_t)recptr < unwindend;
	    rec_cnt++) {
		nextrecp = unwind_decode_R1(recptr, &strc[rec_cnt].udesc);
		if (nextrecp) {
			region_len = strc[rec_cnt].udesc.R1.rlen;
			region_type = strc[rec_cnt].udesc.R1.r;
			strc[rec_cnt].type = R1;
			recptr = nextrecp;
			continue;
		}

		nextrecp = unwind_decode_R2(recptr, &strc[rec_cnt].udesc);
		if (nextrecp) {
			region_len = strc[rec_cnt].udesc.R2.rlen;
			/* R2 regions are prologue regions */
			region_type = false;
			strc[rec_cnt].type = R2;
			recptr = nextrecp;
			continue;
		}

		nextrecp = unwind_decode_R3(recptr, &strc[rec_cnt].udesc);
		if (nextrecp) {
			region_len = strc[rec_cnt].udesc.R3.rlen;
			region_type = strc[rec_cnt].udesc.R3.r;
			strc[rec_cnt].type = R3;
			recptr = nextrecp;
			continue;
		}

		if (region_type == false) {
			/* Prologue Region */
			nextrecp = unwind_decode_P1(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P1;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P2(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P2;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P3(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P3;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P4(recptr,
						    &strc[rec_cnt].udesc,
						    region_len);
			if (nextrecp) {
				strc[rec_cnt].type = P4;
				recptr = nextrecp;
				break;
			}
		
			nextrecp = unwind_decode_P5(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P5;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P6(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P6;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P7(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P7;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P8(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P8;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P9(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P9;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_P10(recptr,
						     &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = P10;
				recptr = nextrecp;
				continue;
			}

			printf("Skipping prologue desc slot :: %d \n", rec_cnt);
		} else {

			nextrecp = unwind_decode_B1(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = B1;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_B2(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = B2;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_B3(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = B3;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_B4(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = B4;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_X1(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = X1;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_X2(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = X2;
				recptr = nextrecp;
				continue;
			}


			nextrecp = unwind_decode_X3(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = X3;
				recptr = nextrecp;
				continue;
			}

			nextrecp = unwind_decode_X4(recptr,
						    &strc[rec_cnt].udesc);
			if (nextrecp) {
				strc[rec_cnt].type = X4;
				recptr = nextrecp;
				continue;
			}

			printf("Skipping body desc slot :: %d \n", rec_cnt);
		}
	}

#ifdef UNWIND_DIAGNOSTIC
	int i;
	for(i = 0;i < rec_cnt;i++) {
		dump_recordchain(&strc[i]);
	}

#endif /* UNWIND_DIAGNOSTIC */
}




/*
 * Debug support: dump a record chain entry
 */
void
dump_recordchain(struct recordchain *rchain)
{

	switch(rchain->type) {
	case R1:
		printf("\t R1:");
		if(rchain->udesc.R1.r)
			printf("body (");
		else
			printf("prologue (");
		printf("rlen = %ld) \n", rchain->udesc.R1.rlen);
		break;

	case R2:
		printf("\t R2:");
		printf("prologue_gr (");
		printf("mask = %x, ", rchain->udesc.R2.mask);
		printf("grsave = %d, ", rchain->udesc.R2.grsave);
		printf("rlen = %ld )\n", rchain->udesc.R2.rlen);
		break;

	case R3:
		printf("\t R3:");
		if(rchain->udesc.R3.r)
			printf("body (");
		else
			printf("prologue (");
		printf("rlen = %ld )\n", rchain->udesc.R3.rlen);
		break;

	case P1:
		printf("\t\tP1:");
		printf("br_mem (brmask = %x) \n", rchain->udesc.P1.brmask);
		break;

	case P2:
		printf("\t\tP2:");
		printf("br_gr(brmask = %x, ", rchain->udesc.P2.brmask);
		printf("gr = %d ) \n", rchain->udesc.P2.gr);
		break;

	case P3:
		printf("\t\tP3:");
		switch(rchain->udesc.P3.r) {
		case 0:
			printf("psp_gr");
			break;
		case 1:
			printf("rp_gr");
			break;
		case 2:
			printf("pfs_gr");
			break;
		case 3:
			printf("preds_gr");
			break;
		case 4:
			printf("unat_gr");
			break;
		case 5:
			printf("lc_gr");
			break;
		case 6:
			printf("rp_br");
			break;
		case 7:
			printf("rnat_gr");
			break;
		case 8:
			printf("bsp_gr");
			break;
		case 9:
			printf("bspstore_gr");
			break;
		case 10:
			printf("fpsr_gr");
			break;
		case 11:
			printf("priunat_gr");
			break;
		default:
			printf("unknown desc: %d", rchain->udesc.P3.r);
				
		}
		printf("(gr/br = %d) \n", rchain->udesc.P3.grbr);

		break;

	case P4:
		printf("P4: (unimplemented): \n");
		break;

	case P5:
		printf("\t\tP5:");
		printf("frgr_mem(grmask = %x, frmask = %x )\n", 
		       rchain->udesc.P5.grmask, rchain->udesc.P5.frmask);
		break;

	case P6:
		printf("\t\tP6: ");
		if(rchain->udesc.P6.r)
			printf("gr_mem( ");
		else
			printf("fr_mem( ");
		printf("rmask = %x) \n", rchain->udesc.P6.rmask);
		break;

	case P7:
		printf("\t\tP7:");
		switch(rchain->udesc.P7.r) {
		case 0:
			printf("memstack_f( ");
			printf("t = %ld, ", rchain->udesc.P7.t);
			printf("size = %ld) \n", rchain->udesc.P7.size);
			break;
		case 1:
			printf("memstack_v( ");
			printf("t = %ld) \n", rchain->udesc.P7.t);
			break;
		case 2:
			printf("spillbase( ");
			printf("pspoff = %ld) \n", rchain->udesc.P7.t);
			break;
		case 3:
			printf("psp_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P7.t);
			break;
		case 4:
			printf("rp_when( ");
			printf("t = %ld) \n", rchain->udesc.P7.t);
			break;
		case 5:
			printf("rp_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P7.t);
			break;
		case 6:
			printf("pfs_when( ");
			printf("t = %ld) \n", rchain->udesc.P7.t);
			break;
		case 7:
			printf("pfs_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P7.t);
			break;
		case 8:
			printf("preds_when( ");
			printf("t = %ld) \n", rchain->udesc.P7.t);
			break;
		case 9:
			printf("preds_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P7.t);
			break;
		case 10:
			printf("lc_when( ");
			printf("t = %ld) \n", rchain->udesc.P7.t);
			break;
		case 11:
			printf("lc_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P7.t);
			break;
		case 12:
			printf("unat_when( ");
			printf("t = %ld) \n", rchain->udesc.P7.t);
			break;
		case 13:
			printf("unat_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P7.t);
			break;
		case 14:
			printf("fpsr_when( ");
			printf("t = %ld) \n", rchain->udesc.P7.t);
			break;
		case 15:
			printf("fpsr_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P7.t);
			break;
		default:
			printf("unknown \n");
		}

		break;
			
	case P8:
		printf("\t\tP8:");
		switch(rchain->udesc.P8.r) {
		case 1:
			printf("rp_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 2:
			printf("pfs_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 3:
			printf("preds_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 4:
			printf("lc_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 5:
			printf("unat_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 6:
			printf("fpsr_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 7:
			printf("bsp_when( ");
			printf("t = %ld) \n", rchain->udesc.P8.t);
			break;
		case 8:
			printf("bsp_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 9:
			printf("bsp_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 10:
			printf("bspstore_when( ");
			printf("t = %ld) \n", rchain->udesc.P8.t);
			break;
		case 11:
			printf("bspstore_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 12:
			printf("bspstore_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 13:
			printf("rnat_when( ");
			printf("t = %ld) \n", rchain->udesc.P8.t);
			break;
		case 14:
			printf("rnat_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 15:
			printf("rnat_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 16:
			printf("priunat_when_gr( ");
			printf("t = %ld) \n", rchain->udesc.P8.t);
			break;
		case 17:
			printf("priunat_psprel( ");
			printf("pspoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 18:
			printf("priunat_sprel( ");
			printf("spoff = %ld) \n", rchain->udesc.P8.t);
			break;
		case 19:
			printf("priunat_when_mem( ");
			printf("t = %ld) \n", rchain->udesc.P8.t);
			break;

		default:
			printf("unknown \n");
		}

		break;

	case P9:
		printf("\t\tP9:");
		printf("(grmask = %x, gr = %d) \n", 
		       rchain->udesc.P9.grmask, rchain->udesc.P9.gr);
		break;

	case P10:
		printf("\t\tP10:");
		printf("(abi: ");
		switch(rchain->udesc.P10.abi) {
		case 0:
			printf("Unix SVR4) \n");
			break;
		case 1:
			printf("HP-UX) \n");
			break;
		default:
			printf("Other) \n");
		}
		break;

	case B1:
		printf("\t\tB1:");
		if(rchain->udesc.B1.r)
			printf("copy_state( ");
		else
			printf("label_state( ");
		printf("label = %d) \n", rchain->udesc.B1.label);

		break;

	case B2:
		printf("\t\tB2:");
		printf("(ecount = %d, t = %ld)\n",
		       rchain->udesc.B2.ecount, rchain->udesc.B2.t);

		break;

	case B3:
		printf("\t\tB3:");
		printf("(t = %ld, ecount = %ld) \n",
		       rchain->udesc.B3.t, rchain->udesc.B3.ecount);

		break;

	case B4:
		printf("\t\tB4:");
		if(rchain->udesc.B4.r)
			printf("copy_state( ");
		else
			printf("label_state( ");

		printf("label = %ld) \n", rchain->udesc.B4.label);

		break;


	case X1:
		printf("\tX1:\n ");
		break;

	case X2:
		printf("\tX2:\n");
		break;

	case X3:
		printf("\tX3:\n");
		break;

	case X4:
		printf("\tX4:\n");
		break;
	default:
		printf("\tunknow: \n");
	}

}

/*
 * State record stuff..... based on section 11. and Appendix A. of the 
 * "Itanium Software Conventions and Runtime Architecture Guide"
 */


/*
 * Global variables:
 * 1. Two arrays of staterecords: recordstack[], recordstackcopy[]
 *	XXX:	Since we don't use malloc, we have two arbitrary sized arrays
 *		providing guaranteed memory from the BSS. See the TODO file 
 *		for more details.
 * 2. Two head variables to hold the member index: unwind_rsp,unwind_rscp
 */

struct staterecord recordstack[MAXSTATERECS];
struct staterecord recordstackcopy[MAXSTATERECS];
struct staterecord current_state;
struct staterecord *unwind_rsp, *unwind_rscp;


/* Base of spill area in memory stack frame as a psp relative offset */
uint64_t spill_base = 0;

/*
 * Initialises a staterecord from a given template,
 * with default values as described by the Runtime Spec.
 */
void
initrecord(struct staterecord *target)
{
	target->bsp.where = UNSAVED;
	target->bsp.when = 0;
	target->bsp.offset = INVALID;
	target->psp.where = UNSAVED;
	target->psp.when = 0;
	target->psp.offset = INVALID;
	target->rp.where = UNSAVED;
	target->rp.when = 0;	
	target->rp.offset = INVALID;
	target->pfs.where = UNSAVED;
	target->pfs.when = 0;
	target->pfs.offset = INVALID;
}


/*
 * Modifies a staterecord structure by parsing 
 * a single record chain structure.
 * regionoffset is the offset within a (prologue) region 
 * where the stack unwinding began.
 */
void
modifyrecord(struct staterecord *srec, struct recordchain *rchain, 
	uint64_t regionoffset)
{
	/*
	 * Default start save GR for prologue_save GRs.
	 */
	uint64_t grno = 32;


	switch (rchain->type) {
	
	case R2:
		/*
		 * R2, prologue_gr is the only region encoding
		 * with register save info.
		 */

		grno = rchain->udesc.R2.grsave;

		if (rchain->udesc.R2.mask & R2MASKRP) {
			srec->rp.when = 0;
			srec->rp.where = GRREL;
			srec->rp.offset = grno++;
		}

		if (rchain->udesc.R2.mask & R2MASKPFS) {
			srec->pfs.when = 0;
			srec->pfs.where = GRREL;
			srec->pfs.offset = grno++;
		}

		if (rchain->udesc.R2.mask & R2MASKPSP) {
			srec->psp.when = 0;
			srec->psp.where = GRREL;
			srec->psp.offset = grno++;
		}
		break;

	case P3:
		switch (rchain->udesc.P3.r) {
		case 0: /* psp_gr */
			if (srec->psp.when < regionoffset) {
				srec->psp.where = GRREL;
				srec->psp.offset = rchain->udesc.P3.grbr;
			}
			break;

		case 1: /* rp_gr */
			if (srec->rp.when < regionoffset) {
				srec->rp.where = GRREL;
				srec->rp.offset = rchain->udesc.P3.grbr;
			}
			break;

		case 2: /* pfs_gr */
			if (srec->pfs.when < regionoffset) {
				srec->pfs.where = GRREL;
				srec->pfs.offset = rchain->udesc.P3.grbr;
			}
			break;

		}
		break;


	/*
	 * XXX: P4 spill_mask and P7: spill_base are for GRs, FRs, and BRs. 
	 * We're not particularly worried about those right now.
	 */

	case P7:
		switch (rchain->udesc.P7.r) {

		case 0: /* mem_stack_f */
			if (srec->psp.offset != INVALID) {
				printf("!!!saw mem_stack_f more than once. \n");
			}
			srec->psp.when = rchain->udesc.P7.t;
			if (srec->psp.when < regionoffset) {
				srec->psp.where = IMMED;
				/* spsz.offset is "overloaded" */
				srec->psp.offset = rchain->udesc.P7.size;
			}
			break;

		case 1: /* mem_stack_v */
			srec->psp.when = rchain->udesc.P7.t;
			break;

		case 2: /* spill_base */
			spill_base = rchain->udesc.P7.t;
			break;

		case 3: /* psp_sprel */
			if (srec->psp.when < regionoffset) {
				srec->psp.where = SPREL;
				srec->psp.offset = rchain->udesc.P7.t;
			}
			break;
			
		case 4: /* rp_when */
			srec->rp.when = rchain->udesc.P7.t;
			/*
			 * XXX: Need to set to prologue_gr(grno) for
			 *	the orphan case ie; _gr/_psprel/_sprel
			 *	not set and therefore default to begin
			 *	from the gr specified in prologue_gr.
			 */
			break;

		case 5: /* rp_psprel */
			if (srec->rp.when < regionoffset) {
				srec->rp.where = PSPREL;
				srec->rp.offset = rchain->udesc.P7.t;
			}
			break;

		case 6: /* pfs_when */
			srec->pfs.when = rchain->udesc.P7.t;
			/*
			 * XXX: Need to set to prologue_gr(grno) for
			 *	the orphan case ie; _gr/_psprel/_sprel
			 *	not set and therefore default to begin
			 *	from the gr specified in prologue_gr.
			 */
			break;

		case 7: /* pfs_psprel */
			if (srec->pfs.when < regionoffset) {
				srec->pfs.where = PSPREL;
				srec->pfs.offset = rchain->udesc.P7.t;
			}
			break;

		}
		break;

	case P8:
		switch (rchain->udesc.P8.r) {
		case 1: /* rp_sprel */
			if (srec->rp.when < regionoffset) {
				srec->rp.where = SPREL;
				srec->rp.offset = rchain->udesc.P8.t;
			}
			break;
		case 2: /* pfs_sprel */
			if (srec->pfs.when < regionoffset) {
				srec->pfs.where = SPREL;
				srec->pfs.offset = rchain->udesc.P8.t;

			}
			break;
		}
		break;

	case B1:

		rchain->udesc.B1.r ? switchrecordstack(0) :
			clonerecordstack(0);
		break;

	case B2:
		if (regionoffset < rchain->udesc.B2.t) {
		poprecord(&current_state, rchain->udesc.B2.ecount);
		}
		break;
	case B3:
		if (regionoffset < rchain->udesc.B3.t) {
		poprecord(&current_state, rchain->udesc.B3.ecount);
		}
		break;
	case B4:
		rchain->udesc.B4.r ? switchrecordstack(0) :
			clonerecordstack(0);
		break;

	case X1:
	case X2:
	case X3:
		/* XXX: Todo */
		break;


	case R1:
	case R3:
	case P1:
	case P2:
	case P4:
	case P5:
	case P6:
	case P9:
	case P10:
	default:
		/* Ignore. */
		printf("XXX: Ignored. \n");
	}


}

void
dump_staterecord(struct staterecord *srec)
{
	printf("rp.where: ");
	switch(srec->rp.where) {
	case UNSAVED:
		printf("UNSAVED ");
		break;
	case BRREL:
		printf("BRREL ");
		break;
	case GRREL:
		printf("GRREL ");
		break;
	case SPREL:
		printf("SPREL ");
		break;
	case PSPREL:
		printf("PSPSREL ");
		break;
	default:
		printf("unknown ");
	}

	printf(", rp.when = %lu, ", srec->rp.when);
	printf("rp.offset = %lu \n", srec->rp.offset);


	printf("pfs.where: ");
	switch(srec->pfs.where) {
	case UNSAVED:
		printf("UNSAVED ");
		break;
	case BRREL:
		printf("BRREL ");
		break;
	case GRREL:
		printf("GRREL ");
		break;
	case SPREL:
		printf("SPREL ");
		break;
	case PSPREL:
		printf("PSPSREL ");
		break;
	default:
		printf("unknown ");
	}

	printf(", pfs.when = %lu, ", srec->pfs.when);
	printf("pfs.offset = %lu \n", srec->pfs.offset);
}


/*
 * Push a state record on the record stack.
 */
void
pushrecord(struct staterecord *srec)
{
	if(unwind_rsp >= recordstack + MAXSTATERECS) {
		printf("Push exceeded array size!!! \n");
		return;
	}

	memcpy(unwind_rsp, srec, sizeof(struct staterecord));
	unwind_rsp++;

}

/*
 * Pop n state records off the record stack.
 */
void
poprecord(struct staterecord *srec, int n)
{
	if(unwind_rsp == recordstack) {
		printf("Popped beyond end of Stack!!! \n");
		return;
	}
	unwind_rsp -= n;
	memcpy(srec, unwind_rsp, sizeof(struct staterecord));
#ifdef DEBUG
	memset(unwind_rsp, 0, sizeof(struct staterecord) * n);
#endif

}

/*
 * Clone the whole record stack upto this one.
 */
void
clonerecordstack(u_int label)
{
	memcpy(recordstackcopy, recordstack, 
	       (unwind_rsp - recordstack) * sizeof(struct staterecord));
	unwind_rscp = unwind_rsp;
}

/*
 * Discard the current stack, and adopt a clone.
 */
void
switchrecordstack(u_int label)
{
	memcpy((void *) recordstack, (void *) recordstackcopy, 
	       (unwind_rscp - recordstackcopy) * sizeof(struct staterecord));
	unwind_rsp = unwind_rscp;

}

/*
 * In the context of a procedure:
 * Parses through a record chain, building, pushing and/or popping staterecords,
 * or cloning/destroying stacks of staterecords as required.
 * Parameters are:
 *	rchain: pointer to recordchain array.
 *	procoffset: offset of point of interest, in slots, within procedure
 *                  starting from slot 0
 * This routine obeys [1]
 */
struct staterecord *
buildrecordstack(struct recordchain *rchain, uint64_t procoffset)
{
	/* Current region length, defaults to zero if not specified */
	uint64_t rlen = 0;
	/* Accumulated region length */
	uint64_t roffset = 0;
	/* Offset within current region */
	uint64_t rdepth = 0;

	bool rtype;
	int i;

	/* Start with bottom of staterecord stack. */
	unwind_rsp = recordstack;

	initrecord(&current_state);

	for (i = 0;i < rec_cnt;i++) {

		switch (rchain[i].type) {
		case R1:
			rlen = rchain[i].udesc.R1.rlen;
			if (procoffset < roffset) {
				/*
				 * Overshot Region containing
				 * procoffset. Bail out.
				 */
				goto out;
			}
			rdepth = procoffset - roffset;
			roffset += rlen;
			rtype = rchain[i].udesc.R1.r;
			if (!rtype) {
				pushrecord(&current_state);
			}
			break;

		case R3:
			rlen = rchain[i].udesc.R3.rlen;
			if (procoffset < roffset) {
				/*
				 * Overshot Region containing
				 * procoffset. Bail out.
				 */
				goto out;
			}
			rdepth = procoffset - roffset;
			roffset += rlen;
			rtype = rchain[i].udesc.R3.r;
			if (!rtype) {
				pushrecord(&current_state);
			}
			break;

		case R2:
			rlen = rchain[i].udesc.R2.rlen;
			if (procoffset < roffset) {
				/*
				 * Overshot Region containing
				 * procoffset. Bail out.
				 */
				goto out;
			}
			rdepth = procoffset - roffset;
			roffset += rlen;
			rtype = false; /* prologue region */
			pushrecord(&current_state);

			/* R2 has save info. Continue down. */
			/* FALLTHROUGH */
		case P1:
		case P2:
		case P3:
		case P4:
		case P5:
		case P6:
		case P7:
		case P8:
		case P9:
		case P10:
			modifyrecord(&current_state, &rchain[i], rdepth);
			break;

		case B1:
		case B2:
		case B3:
		case B4:
			modifyrecord(&current_state, &rchain[i],
				     rlen - 1 - rdepth); 
			break;

		case X1:
		case X2:
		case X3:
		case X4:
		default:
			printf("Error: Unknown descriptor type!!! \n");

		}

#if UNWIND_DIAGNOSTIC
		dump_staterecord(&current_state);
#endif


	}

out:

	return &current_state;
}

void
updateregs(struct unwind_frame *uwf, struct staterecord *srec,
	uint64_t procoffset) 
{
	/* XXX: Update uwf for regs other than rp and pfs*/
	uint64_t roffset = 0;

	/* Uses shadow arrays to update uwf from srec in a loop. */
	/* Count of number of regstate elements in struct staterecord */
	int statecount = sizeof(struct staterecord)/sizeof(struct regstate);	
	/* Pointer to current regstate. */
	struct regstate *stptr = (void *) srec;				
	/* Pointer to current unwind_frame element */
	uint64_t *gr = (void *) uwf;

	int i;

#ifdef UNWIND_DIAGNOSTIC
	printf("updateregs(): \n");
	printf("procoffset (slots) = %lu \n", procoffset);
#endif

	for(i = 0; i < statecount; i++) {
		switch (stptr[i].where) { 
		case IMMED: /* currently only mem_stack_f */
			if (stptr[i].when >= procoffset) break;
			uwf->psp -= (stptr[i].offset << 4); 
			break;

		case GRREL:
			if (stptr[i].when >= procoffset) break;
		
			roffset = stptr[i].offset;
			if (roffset == 0) {
				gr[i] = 0;
				break;
			}


			if (roffset < 32) {
				printf("GR%ld: static register save ??? \n",
				       roffset);
				break;
			}

			/* Fetch from bsp + offset - 32 + Adjust for RNAT. */
			roffset -= 32;
			gr[i] = ia64_getrse_gr(uwf->bsp, roffset);
			break;

		case SPREL:
			if (stptr[i].when >= procoffset) break;

			/* Check if frame has been setup. */
			if (srec->psp.offset == INVALID) {
				printf("sprel used without setting up "
				       "stackframe!!! \n");
				break;
			}

			roffset = stptr[i].offset;

			/* Fetch from sp + offset */
			memcpy(&gr[i], (char *) uwf->sp + roffset * 4,
			       sizeof(uint64_t));
			break;


		case PSPREL:
			if (stptr[i].when >= procoffset) break;

			/* Check if frame has been setup. */
			if (srec->psp.offset == INVALID) {
				printf("psprel used without setting up "
				       "stackframe!!! \n");
				break;
			}

			roffset = stptr[i].offset;

			/* Fetch from sp + offset */
			memcpy(&gr[i], (char *) uwf->psp + 16 - (roffset * 4),
			       sizeof(uint64_t));
			break;

		case UNSAVED:
		case BRREL:
		default:
#ifdef UNWIND_DIAGNOSTIC
			printf ("updateregs: reg[%d] is UNSAVED \n", i);
#endif
			break;
			/* XXX: Not implemented yet. */
		}

	}		

}


/*
 * Locates unwind table entry, given unwind table entry info.
 * Expects the variables ia64_unwindtab, and ia64_unwindtablen
 * to be set appropriately.
 */
struct uwtable_ent *
get_unwind_table_entry(uint64_t iprel)
{

	extern uint64_t ia64_unwindtab, ia64_unwindtablen;

	struct uwtable_ent *uwt;
	int tabent;


	for(uwt = (struct uwtable_ent *) ia64_unwindtab, tabent = 0; 
	    /* The Runtime spec tells me the table entries are sorted. */
	    uwt->end <= iprel && tabent < ia64_unwindtablen;
	    uwt++, tabent += sizeof(struct uwtable_ent));


	if (!(uwt->start <= iprel && iprel < uwt->end)) {
#ifdef UNWIND_DIAGNOSTIC
		printf("Entry not found \n");
		printf("iprel = %lx \n", iprel);
		printf("uwt->start = %lx \nuwt->end = %lx \n",
		       uwt->start, uwt->end);
		printf("tabent = %d \n", tabent);
		printf("ia64_unwindtablen = %ld \n", 
		       ia64_unwindtablen);
#endif
		return NULL;
	}

#ifdef UNWIND_DIAGNOSTIC
	printf("uwt->start = %lx \nuwt->end = %lx \n"
	       "uwt->infoptr = %p\n", uwt->start, uwt->end, uwt->infoptr);
#endif

	return uwt;
}


/* 
 * Reads unwind table info and updates register values.
 */
void 
patchunwindframe(struct unwind_frame *uwf, uint64_t iprel, uint64_t relocoffset)
{

	extern struct recordchain strc[];
	struct staterecord *srec;
	struct uwtable_ent *uwt;
	uint64_t infoptr, procoffset, slotoffset;

#if 0 /* does not work - moved to assertion at the call site */
	if (iprel < 0) {
		panic("unwind ip out of range!!! \n");
		return;
	}
#endif

	uwt = get_unwind_table_entry(iprel);

	if (uwt == NULL) return;

	infoptr = (uint64_t) uwt->infoptr + relocoffset;

	if (infoptr > relocoffset) {
		buildrecordchain(infoptr, NULL);
	} else {
		return;
	}
	
	slotoffset = iprel & 3;

	/* procoffset in Number of _slots_ , _not_ a byte offset. */

	procoffset = (((iprel - slotoffset) - (uwt->start)) / 0x10 * 3)
		+ slotoffset; 
	srec = buildrecordstack(strc, procoffset);

	updateregs(uwf, srec, procoffset);
}
