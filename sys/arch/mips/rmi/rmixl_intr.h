/*	$NetBSD: rmixl_intr.h,v 1.1.2.1 2010/03/21 19:28:01 cliff Exp $	*/

#ifndef _MIPS_RMI_RMIXL_INTR_H_
#define _MIPS_RMI_RMIXL_INTR_H_

/*
 * An 'irq' is an EIRR bit numbers or 'vector' as used in the PRM
 * - PIC-based irqs are in the range 0..31 and index into the IRT
 * - IRT entry <n> always routes to vector <n>
 * - non-PIC-based irqs are in the range 32..63
 * - only 1 intrhand_t per irq/vector
 */
#define	NINTRVECS	64	/* bit width of the EIRR */
#define	NIRTS		32	/* #entries in the Interrupt Redirection Table */

/*
 * reserved vectors >=32
 */
#define RMIXL_INTRVEC_IPI	32
#define RMIXL_INTRVEC_FMN	33

typedef enum {
	RMIXL_TRIG_NONE=0,
	RMIXL_TRIG_EDGE,
	RMIXL_TRIG_LEVEL,
} rmixl_intr_trigger_t;

typedef enum {
	RMIXL_POLR_NONE=0,
	RMIXL_POLR_RISING,
	RMIXL_POLR_HIGH,
	RMIXL_POLR_FALLING,
	RMIXL_POLR_LOW,
} rmixl_intr_polarity_t;


/*
 * iv_list and ref count manage sharing of each vector
 */
typedef struct rmixl_intrhand {
        int (*ih_func)(void *);
        void *ih_arg; 
        int ih_irq;			/* >=32 if not-PIC-based */
        int ih_ipl; 			/* interrupt priority */
        int ih_cpumask; 		/* CPUs which may handle this irpt */
} rmixl_intrhand_t;

/*
 * stuff exported from rmixl_spl.S
 */
extern const struct splsw rmixl_splsw;
extern uint64_t ipl_eimr_map[];

extern void *rmixl_intr_establish(int, int, int,
	rmixl_intr_trigger_t, rmixl_intr_polarity_t,
	int (*)(void *), void *);
extern void  rmixl_intr_disestablish(void *);
extern void *rmixl_vec_establish(int, int, int,
	int (*)(void *), void *);
extern void  rmixl_vec_disestablish(void *);
extern const char *rmixl_intr_string(int);
extern void rmixl_intr_init_cpu(struct cpu_info *);
extern void *rmixl_intr_init_clk(void);
#ifdef MULTIPROCESSOR
extern void *rmixl_intr_init_ipi(void);
#endif

#endif	/* _MIPS_RMI_RMIXL_INTR_H_ */
