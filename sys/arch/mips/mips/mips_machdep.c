/*	$NetBSD: mips_machdep.c,v 1.12 1997/06/19 06:30:08 mhitch Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <mips/cpu.h>		/* declaration of of cpu_id */
#include <mips/locore.h>
#include <machine/cpu.h>		/* declaration of of cpu_id */

mips_locore_jumpvec_t mips_locore_jumpvec = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL
};

/*
 * Forward declarations
 * XXX should be in a header file so each mips port can include it.
 */
extern void cpu_identify __P((void));
extern void mips_vector_init __P((void));

void mips1_vector_init __P((void));
void mips3_vector_init __P((void));

/* the following is used externally (sysctl_hw) */
char	machine_arch[] = MACHINE_ARCH;	/* cpu "architecture" */

#ifdef MIPS1	/*  r2000 family  (mips-I cpu) */
/*
 * MIPS-I (r2000) locore-function vector.
 */
mips_locore_jumpvec_t R2000_locore_vec =
{
	mips1_ConfigCache,
	mips1_FlushCache,
	mips1_FlushDCache,
	mips1_FlushICache,
	/*mips1_FlushICache*/ mips1_FlushCache,
	mips1_SetPID,
	mips1_TLBFlush,
	mips1_TLBFlushAddr,
	mips1_TLBUpdate,
	mips1_TLBWriteIndexed,
	mips1_wbflush,
	mips1_proc_trampoline,
	mips1_switch_exit,
	mips1_cpu_switch_resume
};

void
mips1_vector_init()
{
	extern char mips1_UTLBMiss[], mips1_UTLBMissEnd[];
	extern char mips1_exception[], mips1_exceptionEnd[];

	/*
	 * Copy down exception vector code.
	 */
	if (mips1_UTLBMissEnd - mips1_UTLBMiss > 0x80)
		panic("startup: UTLB code too large");
	bcopy(mips1_UTLBMiss, (char *)MACH_UTLB_MISS_EXC_VEC,
		mips1_UTLBMissEnd - mips1_UTLBMiss);
	bcopy(mips1_exception, (char *)MIPS1_GEN_EXC_VEC,
	      mips1_exceptionEnd - mips1_exception);

	/*
	 * Copy locore-function vector.
	 */
	bcopy(&R2000_locore_vec, &mips_locore_jumpvec,
	      sizeof(mips_locore_jumpvec_t));

	/*
	 * Clear out the I and D caches.
	 */
	mips1_ConfigCache();
	mips1_FlushCache();
}
#endif /* MIPS1 */


#ifdef MIPS3		/* r4000 family (mips-III cpu) */
/*
 * MIPS-III (r4000) locore-function vector.
 */
mips_locore_jumpvec_t R4000_locore_vec =
{
	mips3_ConfigCache,
	mips3_FlushCache,
	mips3_FlushDCache,
	mips3_FlushICache,
#if 0
	 /*
	  * No such vector exists, perhaps it was meant to be HitFlushDCache?
	  */
	mips3_ForceCacheUpdate,
#else
	mips3_FlushCache,
#endif
	mips3_SetPID,
	mips3_TLBFlush,
	mips3_TLBFlushAddr,
	mips3_TLBUpdate,
	mips3_TLBWriteIndexed,
	mips3_wbflush,
	mips3_proc_trampoline,
	mips3_switch_exit,
	mips3_cpu_switch_resume
};

void
mips3_vector_init()
{

	/* r4000 exception handler address and end */
	extern char mips3_exception[], mips3_exceptionEnd[];

	/* TLB miss handler address and end */
	extern char mips3_TLBMiss[], mips3_TLBMissEnd[];

	/*
	 * Copy down exception vector code.
	 */
#if 0  /* XXX not restricted? */
	if (mips3_TLBMissEnd - mips3_TLBMiss > 0x80)
		panic("startup: UTLB code too large");
#endif
	bcopy(mips3_TLBMiss, (char *)MACH_UTLB_MISS_EXC_VEC,
	      mips3_TLBMissEnd - mips3_TLBMiss);

	bcopy(mips3_exception, (char *)MIPS3_GEN_EXC_VEC,
	      mips3_exceptionEnd - mips3_exception);

	/*
	 * Copy locore-function vector.
	 */
	bcopy(&R4000_locore_vec, &mips_locore_jumpvec,
	      sizeof(mips_locore_jumpvec_t));

	/*
	 * Clear out the I and D caches.
	 */
	mips3_ConfigCache();
	mips3_FlushCache();
}
#endif	/* MIPS3 */


/*
 * Do all the stuff that locore normally does before calling main(),
 * that is common to all mips-CPU NetBSD ports.
 *
 * The principal purpose of this function is to examine the
 * variable cpu_id, into which the kernel locore start code
 * writes the cpu ID register, and to then copy appropriate
 * cod into the CPU exception-vector entries and the jump tables
 * used to  hide the differences in cache and TLB handling in
 * different MIPS  CPUs.
 * 
 * This should be the very first thing called by each port's
 * init_main() function.
 */

/*
 * Initialize the hardware exception vectors, and the jump table used to
 * call locore cache and TLB management functions, based on the kind
 * of CPU the kernel is running on.
 */
void
mips_vector_init()
{


#ifdef notyet
	register caddr_t v;
	extern char edata[], end[];

	/* clear the BSS segment */
	v = (caddr_t)mips_round_page(end);
	bzero(edata, v - edata);
#endif
	int i;

	(void) &i;		/* shut off gcc unused-variable warnings */

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */

	switch (cpu_id.cpu.cp_imp) {
#ifdef MIPS1
	case MIPS_R2000:
	case MIPS_R3000:
		cpu_arch = 1;
		mips1_TLBFlush();
		for (i = 0; i < VMMACH_MIPS_3K_FIRST_RAND_ENTRY; ++i)
			mips1_TLBWriteIndexed(i, MACH_CACHED_MEMORY_ADDR, 0);
		mips1_vector_init();
		break;
#endif
#ifdef MIPS3
	case MIPS_R4000:
		cpu_arch = 3;
		mips3_SetWIRED(0);
		mips3_TLBFlush();
		mips3_SetWIRED(VMMACH_WIRED_ENTRIES);
		mips3_vector_init();
		break;
#endif

	default:
		printf("CPU type (%d) not supported\n", cpu_id.cpu.cp_imp);
		cpu_reboot(RB_HALT, NULL);
	}
}


/*
 * Identify cpu and fpu type and revision.
 *
 * XXX Should be moved to mips_cpu.c but that doesn't exist
 */
void
cpu_identify()
{


	/* Work out what kind of CPU and FPU are present. */

	switch(cpu_id.cpu.cp_imp) {

	case MIPS_R2000:
		printf("MIPS R2000 CPU");
		break;
	case MIPS_R3000:

	  	/*
		 * XXX
		 * R2000A silicion has an r3000 core and shows up here.
		 *  The caller should indicate that by setting a flag
		 * indicating the baseboard is  socketed for an r2000.
		 * Needs more thought.
		 */
#ifdef notyet
	  	if (SYSTEM_HAS_R2000_CPU_SOCKET())
			printf("MIPS R2000A CPU");
		else
#endif
		printf("MIPS R3000 CPU");
		break;
	case MIPS_R6000:
		printf("MIPS R6000 CPU");
		break;

	case MIPS_R4000:
		if(mips_L1InstCacheSize == 16384)
			printf("MIPS R4400 CPU");
		else
			printf("MIPS R4000 CPU");
		break;
	case MIPS_R3LSI:
		printf("LSI Logic R3000 derivate");
		break;
	case MIPS_R6000A:
		printf("MIPS R6000A CPU");
		break;
	case MIPS_R3IDT:
		printf("IDT R3000 derivate");
		break;
	case MIPS_R10000:
		printf("MIPS R10000/T5 CPU");
		break;
	case MIPS_R4200:
		printf("MIPS R4200 CPU (ICE)");
		break;
	case MIPS_R8000:
		printf("MIPS R8000 Blackbird/TFP CPU");
		break;
	case MIPS_R4600:
		printf("QED R4600 Orion CPU");
		break;
	case MIPS_R3SONY:
		printf("Sony R3000 based CPU");
		break;
	case MIPS_R3TOSH:
		printf("Toshiba R3000 based CPU");
		break;
	case MIPS_R3NKK:
		printf("NKK R3000 based CPU");
		break;
	case MIPS_UNKC1:
	case MIPS_UNKC2:
	default:
		printf("Unknown CPU type (0x%x)",cpu_id.cpu.cp_imp);
		break;
	}
	printf(" Rev. %d.%d with ", cpu_id.cpu.cp_majrev, cpu_id.cpu.cp_minrev);


	switch(fpu_id.cpu.cp_imp) {

	case MIPS_SOFT:
		printf("Software emulation float");
		break;
	case MIPS_R2360:
		printf("MIPS R2360 FPC");
		break;
	case MIPS_R2010:
		printf("MIPS R2010 FPC");
		break;
	case MIPS_R3010:
	  	/*
		 * XXX FPUs  for R2000A(?) silicion has an r3010 core and
		 *  shows up here.
		 */
#ifdef notyet
	  	if (SYSTEM_HAS_R2000_CPU_SOCKET())
			printf("MIPS R2010A CPU");
		else
#endif
		printf("MIPS R3010 FPC");
		break;
	case MIPS_R6010:
		printf("MIPS R6010 FPC");
		break;
	case MIPS_R4010:
		printf("MIPS R4010 FPC");
		break;
	case MIPS_R31LSI:
		printf("FPC");
		break;
	case MIPS_R10010:
		printf("MIPS R10000/T5 FPU");
		break;
	case MIPS_R4210:
		printf("MIPS R4200 FPC (ICE)");
	case MIPS_R8000:
		printf("MIPS R8000 Blackbird/TFP");
		break;
	case MIPS_R4600:
		printf("QED R4600 Orion FPC");
		break;
	case MIPS_R3SONY:
		printf("Sony R3000 based FPC");
		break;
	case MIPS_R3TOSH:
		printf("Toshiba R3000 based FPC");
		break;
	case MIPS_R3NKK:
		printf("NKK R3000 based FPC");
		break;
	case MIPS_UNKF1:
	default:
		printf("Unknown FPU type (0x%x)", fpu_id.cpu.cp_imp);
		break;
	}
	printf(" Rev. %d.%d", fpu_id.cpu.cp_majrev, fpu_id.cpu.cp_minrev);
	printf("\n");

#ifdef MIPS3
	printf("        Primary cache size: %dkb Instruction, %dkb Data, %dkb Secondary.\n",
	    mips_L1InstCacheSize / 1024,
	    mips_L1DataCacheSize / 1024,
	    mips_L2CacheSize / 1024);
#endif
/* XXX cache sizes for MIPS1? */
}
