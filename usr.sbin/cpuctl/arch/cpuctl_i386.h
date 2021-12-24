/*      $NetBSD: cpuctl_i386.h,v 1.4.2.2 2021/12/24 12:58:14 martin Exp $      */

#include <machine/specialreg.h>
#include <x86/cputypes.h>
#include <x86/cacheinfo.h>

struct cpu_info {
	const char	*ci_dev;
	int32_t		ci_cpu_type;	 /* for cpu's without cpuid */
	uint32_t	ci_signature;	 /* X86 cpuid type */
	uint32_t	ci_vendor[4];	 /* vendor string */
	int32_t		ci_max_cpuid;	 /* highest cpuid supported */
	uint32_t	ci_max_ext_cpuid; /* highest cpuid extended func lv */
	uint32_t	ci_family;	 /* from ci_signature */
	uint32_t	ci_model;	 /* from ci_signature */
	uint32_t	ci_feat_val[10]; /* X86 CPUID feature bits
					  *	[0] basic features %edx
					  *	[1] basic features %ecx
					  *	[2] extended features %edx
					  *	[3] extended features %ecx
					  *	[4] VIA padlock features
					  *	[5] structure ext. feat. %ebx
					  *	[6] structure ext. feat. %ecx
					  *     [7] structure ext. feat. %edx
					  *	[8] XCR0 bits (d:0 %eax)
					  *	[9] xsave flags (d:1 %eax)
					  */
	uint32_t	ci_cpu_class;	 /* CPU class */
	uint32_t	ci_brand_id;	 /* Intel brand id */
	uint32_t	ci_cpu_serial[3]; /* PIII serial number */
	uint64_t	ci_tsc_freq;	 /* cpu cycles/second */
	uint8_t		ci_packageid;
	uint8_t		ci_coreid;
	uint8_t		ci_smtid;
	uint32_t	ci_initapicid;	/* our initial APIC ID */

	uint32_t	ci_cur_xsave;
	uint32_t	ci_max_xsave;

	struct x86_cache_info ci_cinfo[CAI_COUNT];
	void		(*ci_info)(struct cpu_info *);
};

extern int cpu_vendor;

/* For x86/x86/identcpu_subr.c */
uint64_t cpu_tsc_freq_cpuid(struct cpu_info *);
void	cpu_dcp_cacheinfo(struct cpu_info *, uint32_t);

/* Interfaces to code in i386-asm.S */

#define	x86_cpuid(a,b)	x86_cpuid2((a),0,(b))

void x86_cpuid2(uint32_t, uint32_t, uint32_t *);
uint32_t x86_identify(void);
uint32_t x86_xgetbv(void);
