/* $NetBSD: powernow_k7.c,v 1.3 2006/01/11 00:18:28 xtraeme Exp $ */

/*
 * Copyright (c) 2004 Martin Végiard.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* AMD PowerNow! K7 driver */

/* Sysctl related code was adapted from NetBSD's i386/est.c for compatibility */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powernow_k7.c,v 1.3 2006/01/11 00:18:28 xtraeme Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <dev/isa/isareg.h>

#include <machine/cpu.h>
#include <machine/isa_machdep.h>

#define BIOS_START 	0xe0000
#define BIOS_END	0x20000
#define BIOS_LEN	BIOS_END - BIOS_START

#define MSR_K7_CTL	0xC0010041
#define CTL_SET_FID	0x0000000000010000ULL
#define CTL_SET_VID	0x0000000000020000ULL

#define cpufreq(x)	fsb * fid_codes[x] / 10

/* pnowk7_init() already declared on i386/include/cpu.h */
#if defined(_LKM)
void pnowk7_destroy(void);
#endif

struct psb_s {
	char signature[10];  	/* AMDK7PNOW! */
	uint8_t version;
	uint8_t flags;
	uint16_t ttime;		/* Min Settling time */
	uint8_t reserved;
	uint8_t n_pst;
};

struct pst_s {
	cpuid_t cpuid;
	uint8_t fsb;		/* Front Side Bus frequency (Mhz) */
	uint8_t fid;		/* Max Frequency code */
	uint8_t vid;		/* Max Voltage code */
	uint8_t n_states;	/* Number of states */
};	

struct state_s {
	uint8_t fid;  		/* Frequency code */
	uint8_t vid;		/* Voltage code */
};

struct freq_table_s {
	unsigned int frequency;
	struct state_s *state;
};

/* Taken from powernow-k7.c/Linux by Dave Jones */
static int fid_codes[32] = {
	110, 115, 120, 125, 50, 55, 60, 65,
	70, 75, 80, 85, 90, 95, 100, 105,
	30, 190, 40, 200, 130, 135, 140, 210,
	150, 225, 160, 165, 170, 180, -1, -1
};

/* Static variables */
static unsigned int fsb;
static unsigned int cur_freq;
static unsigned int ttime;
static unsigned int n_states;
static struct freq_table_s *freq_table;
static int powernow_node_target, powernow_node_current;
static char *freq_names;
#if defined(_LKM)
static struct sysctllog *sysctllog;
#define SYSCTLLOG	&sysctllog
#else
#define SYSCTLLOG	NULL
#endif

/* Prototypes */
struct state_s *pnowk7_getstates(cpuid_t cpuid);
int pnowk7_setstate(unsigned int freq);
static int powernow_sysctl_helper(SYSCTLFN_PROTO);

static int
powernow_sysctl_helper(SYSCTLFN_ARGS)
{
        struct sysctlnode node;
        int fq, oldfq, error;

        if (freq_table == NULL)
                return EOPNOTSUPP;

        node = *rnode;
        node.sysctl_data = &fq;

	oldfq = 0;
        if (rnode->sysctl_num == powernow_node_target)
                fq = oldfq = cur_freq;
        else if (rnode->sysctl_num == powernow_node_current)
                fq = cur_freq;
        else
                return EOPNOTSUPP;

        error = sysctl_lookup(SYSCTLFN_CALL(&node));
        if (error || newp == NULL)
                return error;

	/* support writing to ...frequency.target */
        if (rnode->sysctl_num == powernow_node_target && fq != oldfq) {
		if (pnowk7_setstate(fq) == 0)
			cur_freq = fq;
		else
			aprint_normal("\nInvalid CPU frequency request\n");
        }

        return 0;
}

struct state_s *
pnowk7_getstates(cpuid_t cpuid)
{
	unsigned int i, j;
	char *ptr;
	
	struct psb_s *psb;
	struct pst_s *pst;

	/*
	 * Look in the 0xe0000 - 0x20000 physical address
	 * range for the pst tables; 16 byte blocks
	 */	

	ptr = (char *)ISA_HOLE_VADDR(BIOS_START);

	for (i = 0; i < BIOS_LEN; i += 16, ptr += 16) {
		if (memcmp(ptr, "AMDK7PNOW!", 10) == 0) {	
			psb = (struct psb_s *) ptr;
			ptr += sizeof(struct psb_s);			

			ttime = psb->ttime;
			
			/* Only this version is supported */
			if (psb->version != 0x12)
				return 0;

			/* Find the right PST */
			for (j = 0; j < psb->n_pst; j++) {
				pst = (struct pst_s *) ptr;
				ptr += sizeof(struct pst_s);

				/* Use the first PST with matching CPUID */
				if ((cpuid == pst->cpuid) ||
				    (cpuid == (pst->cpuid - 0x100))) {
				/* 
				 * XXX
				 * some braindead BIOS seems to expect
				 * signature 0x780 from Athlon. Let's try to
				 * subtract 0x100 from the BIOS supplied values
				 * to fix.
				 */
					/*
					 * XXX
					 * I need more info on this.
					 * For now, let's just ignore it
					 */
					if ((cpuid & 0xFF) == 0x60)
						return 0;

					fsb = pst->fsb;
					n_states = pst->n_states;
					return (struct state_s *) ptr;
				} else
					ptr += sizeof(struct state_s) * 
						    pst->n_states;
			}
			
			/* aprint_normal("No match was found for your CPUID\n"); */
			return 0;
		}
	}

	/* aprint_normal("Power state table not found\n"); */
	return 0;
}

int
pnowk7_setstate(unsigned int freq)
{
	unsigned int i;
	u_int32_t sgtc, vid, fid;
	u_int64_t ctl;

	vid = fid = 0;	

	for (i = 0; i < n_states; i++) {
		/* Do we know how to set that frequency? */
		if (freq_table[i].frequency == freq) {
			fid = freq_table[i].state->fid;
			vid = freq_table[i].state->vid;
		}
 	}
	
	if (fid == 0 || vid == 0)
		return -1;

	/* Get CTL and only modify fid/vid/sgtc */
	ctl = rdmsr(MSR_K7_CTL);
	
	/* FID */	
	ctl &= 0xFFFFFFFFFFFFFF00ULL;
	ctl |= fid;

	/* VID */
	ctl &= 0xFFFFFFFFFFFF00FFULL;
	ctl |= vid << 8;

	/* SGTC */
	if ((sgtc = ttime * 100) < 10000) sgtc = 10000;
	ctl &= 0xFFF00000FFFFFFFFULL;
	ctl |= (u_int64_t)sgtc << 32; 

	if (cur_freq > freq) {
		wrmsr(MSR_K7_CTL, ctl | CTL_SET_FID);
		wrmsr(MSR_K7_CTL, ctl | CTL_SET_VID);
	} else {
		wrmsr(MSR_K7_CTL, ctl | CTL_SET_VID);
		wrmsr(MSR_K7_CTL, ctl | CTL_SET_FID);
	}
	
	ctl = rdmsr(MSR_K7_CTL);

	return 0;
}

void
pnowk7_init(struct cpu_info *ci)
{
	int rc;
	unsigned int i, freq_names_len, len = 0;
	char *cpuname;
	const struct sysctlnode *node, *pnownode, *freqnode;
	struct state_s *s = pnowk7_getstates(ci->ci_signature);

	cpuname = ci->ci_dev->dv_xname;

	if (s == 0) {
		/* aprint_normal("AMD PowerNow not supported\n"); */
		return;
	}

	freq_names_len =  n_states * (sizeof("9999 ")-1) + 1;
	freq_names = malloc(freq_names_len, M_SYSCTLDATA, M_WAITOK);
	
	freq_table = malloc(sizeof(struct freq_table_s) * n_states, M_TEMP,
	    M_WAITOK);

	for (i = 0; i < n_states; i++, s++) {		
		freq_table[i].frequency = cpufreq(s->fid);
		freq_table[i].state = s;

                len += snprintf(freq_names + len, freq_names_len - len, "%d%s",
                    freq_table[i].frequency, i < n_states - 1 ? " " : "");
	}

	/* On bootup the frequency should be at its max */
	cur_freq = freq_table[i-1].frequency;	

	/* Create sysctl machdep.powernow.frequency. */
        if ((rc = sysctl_createv(SYSCTLLOG, 0, NULL, &node,
            CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
            NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL)) != 0)
                goto err;

        if ((rc = sysctl_createv(SYSCTLLOG, 0, &node, &pnownode,
            0, CTLTYPE_NODE, "powernow", NULL,
            NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
                goto err;

         if ((rc = sysctl_createv(SYSCTLLOG, 0, &pnownode, &freqnode,
            0, CTLTYPE_NODE, "frequency", NULL,
            NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
                goto err;

        if ((rc = sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
            CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
            powernow_sysctl_helper, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
                goto err;
        powernow_node_target = node->sysctl_num;

        if ((rc = sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
            0, CTLTYPE_INT, "current", NULL,
            powernow_sysctl_helper, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
                goto err;
        powernow_node_current = node->sysctl_num;

        if ((rc = sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
            0, CTLTYPE_STRING, "available", NULL,
            NULL, 0, freq_names, freq_names_len, CTL_CREATE, CTL_EOL)) != 0)
                goto err;

	aprint_normal("%s: AMD PowerNow! Technology supported\n", cpuname);
	aprint_normal("%s: available frequencies (Mhz): %s\n",
	    cpuname, freq_names);
        aprint_normal("%s: current frequency (Mhz): %d\n", cpuname, cur_freq);

	return;

  err:
	aprint_normal("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

#if defined(_LKM)
void
pnowk7_destroy(void)
{
	sysctl_teardown(&sysctllog);
}
#endif
