/* $NetBSD: kvm86.c,v 1.5 2003/01/20 18:43:18 drochner Exp $ */

/*
 * Copyright (c) 2002
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kvm86.c,v 1.5 2003/01/20 18:43:18 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <uvm/uvm.h>
#include <machine/pcb.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/kvm86.h>

/* assembler functions in kvm86call.s */
extern int kvm86_call(struct trapframe *);
extern void kvm86_ret(struct trapframe *, int);

struct kvm86_data {
#define PGTABLE_SIZE	((1024 + 64) * 1024 / NBPG)
	pt_entry_t pgtbl[PGTABLE_SIZE]; /* must be aliged */

	struct segment_descriptor sd;

	struct pcb pcb; /* contains TSS */
	u_long iomap[0x10000/32]; /* full size io permission map */
};

static void kvm86_map(struct kvm86_data *, paddr_t, u_int32_t);
static void kvm86_mapbios(struct kvm86_data *);

/*
 * global VM for BIOS calls
 */
struct kvm86_data *bioscallvmd;
/* page for trampoline and stack */
void *bioscallscratchpage;
/* where this page is mapped in the vm86 */
#define BIOSCALLSCRATCHPAGE_VMVA 0x1000
/* a virtual page to map in vm86 memory temporarily */
vaddr_t bioscalltmpva;

#define KVM86_IOPL3 /* not strictly necessary, saves a lot of traps */

void
kvm86_init()
{
	size_t vmdsize;
	char *buf;
	struct kvm86_data *vmd;
	struct pcb *pcb;
	int i;

	vmdsize = round_page(sizeof(struct kvm86_data)) + NBPG;

	buf = malloc(vmdsize, M_DEVBUF, M_NOWAIT);
	if ((u_long)buf & (NBPG - 1)) {
		printf("struct kvm86_data unaligned\n");
		return;
	}
	memset(buf, 0, vmdsize);
	/* first page is stack */
	vmd = (struct kvm86_data *)(buf + NBPG);
	pcb = &vmd->pcb;

	/*
	 * derive pcb and TSS from proc0
	 * we want to access all IO ports, so we need a full-size
	 *  permission bitmap
	 * XXX do we really need the pcb or just the TSS?
	 */
	memcpy(pcb, &lwp0.l_addr->u_pcb, sizeof(struct pcb));
	pcb->pcb_tss.tss_esp0 = (int)vmd;
	pcb->pcb_tss.tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	for (i = 0; i < sizeof(vmd->iomap) / 4; i++)
		vmd->iomap[i] = 0;
	pcb->pcb_tss.tss_ioopt =
		((caddr_t)vmd->iomap - (caddr_t)&pcb->pcb_tss) << 16;

	/* setup TSS descriptor (including our iomap) */
	setsegment(&vmd->sd, &pcb->pcb_tss,
	    sizeof(struct pcb) + sizeof(vmd->iomap) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);

	/* prepare VM for BIOS calls */
	kvm86_mapbios(vmd);
	bioscallscratchpage = malloc(NBPG, M_DEVBUF, M_NOWAIT);
	kvm86_map(vmd, vtophys((vaddr_t)bioscallscratchpage),
		  BIOSCALLSCRATCHPAGE_VMVA);
	bioscallvmd = vmd;
	bioscalltmpva = uvm_km_valloc(kernel_map, NBPG);
}

/*
 * XXX pass some stuff to the assembler code
 * XXX this should be done cleanly (in call argument to kvm86_call())
 */
static void kvm86_prepare(struct kvm86_data *);
static void
kvm86_prepare(vmd)
	struct kvm86_data *vmd;
{
	extern struct pcb *vm86pcb;
	extern int vm86tssd0, vm86tssd1;
	extern paddr_t vm86newptd;
	extern struct trapframe *vm86frame;
	extern pt_entry_t *vm86pgtableva;

#ifdef MULTIPROCESSOR
#error this needs a rewrite for MP
#endif

	vm86newptd = vtophys((vaddr_t)vmd) | PG_V | PG_RW | PG_U | PG_u;
	vm86pgtableva = vmd->pgtbl;
	vm86frame = (struct trapframe *)vmd - 1;
	vm86pcb = &vmd->pcb;
	vm86tssd0 = *(int*)&vmd->sd;
	vm86tssd1 = *((int*)&vmd->sd + 1);
}

static void
kvm86_map(vmd, pa, vmva)
	struct kvm86_data *vmd;
	paddr_t pa;
	u_int32_t vmva;
{

	vmd->pgtbl[vmva >> 12] = pa | PG_V | PG_RW | PG_U | PG_u;
}

static void
kvm86_mapbios(vmd)
	struct kvm86_data *vmd;
{
	paddr_t pa;

	/* map first physical page (vector table, BIOS data) */
	kvm86_map(vmd, 0, 0);

	/* map ISA hole */
	for (pa = 0xa0000; pa < 0x100000; pa += NBPG)
		kvm86_map(vmd, pa, pa);
}

void *
kvm86_bios_addpage(vmva)
	u_int32_t vmva;
{
	void *mem;

	if (bioscallvmd->pgtbl[vmva >> 12]) /* allocated? */
		return (0);

	mem = malloc(NBPG, M_DEVBUF, M_NOWAIT);
	if ((u_long)mem & (NBPG - 1)) {
		printf("kvm86_bios_addpage: unaligned");
		return (0);
	}
	kvm86_map(bioscallvmd, vtophys((vaddr_t)mem), vmva);

	return (mem);
}

void
kvm86_bios_delpage(vmva, kva)
	u_int32_t vmva;
	void *kva;
{

	bioscallvmd->pgtbl[vmva >> 12] = 0;
	free(kva, M_DEVBUF);
}

size_t
kvm86_bios_read(vmva, buf, len)
	u_int32_t vmva;
	char *buf;
	size_t len;
{
	size_t todo, now;
	paddr_t vmpa;

	todo = len;
	while (todo > 0) {
		now = min(todo, NBPG - (vmva & (NBPG - 1)));

		if (!bioscallvmd->pgtbl[vmva >> 12])
			break;
		vmpa = bioscallvmd->pgtbl[vmva >> 12] & ~(NBPG - 1);
		pmap_kenter_pa(bioscalltmpva, vmpa, VM_PROT_READ);
		pmap_update(pmap_kernel());

		memcpy(buf, (void *)(bioscalltmpva + (vmva & (NBPG - 1))),
		       now);
		buf += now;
		todo -= now;
		vmva += now;
	}
	return (len - todo);
}

int
kvm86_bioscall(intno, tf)
	int intno;
	struct trapframe *tf;
{
	static const unsigned char call[] = {
		0xfa, /* CLI */
		0xcd, /* INTxx */
		0,
		0xfb, /* STI */
		0xf4  /* HLT */
	};

	memcpy(bioscallscratchpage, call, sizeof(call));
	*((unsigned char *)bioscallscratchpage + 2) = intno;

	tf->tf_eip = BIOSCALLSCRATCHPAGE_VMVA;
	tf->tf_cs = 0;
	tf->tf_esp = BIOSCALLSCRATCHPAGE_VMVA + NBPG - 2;
	tf->tf_ss = 0;
	tf->tf_eflags = PSL_USERSET | PSL_VM;
#ifdef KVM86_IOPL3
	tf->tf_eflags |= PSL_IOPL;
#endif
	tf->tf_ds = tf->tf_es = tf->tf_fs = tf->tf_gs = 0;

	kvm86_prepare(bioscallvmd); /* XXX */
	return (kvm86_call(tf));
}

int
kvm86_bioscall_simple(intno, r)
	int intno;
	struct bioscallregs *r;
{
	struct trapframe tf;
	int res;

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = r->EAX;
	tf.tf_ebx = r->EBX;
	tf.tf_ecx = r->ECX;
	tf.tf_edx = r->EDX;
	tf.tf_esi = r->ESI;
	tf.tf_edi = r->EDI;
	tf.tf_vm86_es = r->ES;

	res = kvm86_bioscall(intno, &tf);

	r->EAX = tf.tf_eax;
	r->EBX = tf.tf_ebx;
	r->ECX = tf.tf_ecx;
	r->EDX = tf.tf_edx;
	r->ESI = tf.tf_esi;
	r->EDI = tf.tf_edi;
	r->ES = tf.tf_vm86_es;
	r->EFLAGS = tf.tf_eflags;

	return (res);
}

void
kvm86_gpfault(tf)
	struct trapframe *tf;
{
	unsigned char *kva, insn, trapno;
	u_int16_t *sp;

	kva = (unsigned char *)((tf->tf_cs << 4) + tf->tf_eip);
	insn = *kva;
#ifdef KVM86DEBUG
	printf("kvm86_gpfault: cs=%x, eip=%x, insn=%x, eflags=%x\n",
	       tf->tf_cs, tf->tf_eip, insn, tf->tf_eflags);
#endif

	KASSERT(tf->tf_eflags & PSL_VM);

	switch (insn) {
	case 0xf4: /* HLT - normal exit */
		kvm86_ret(tf, 0);
		break;
	case 0xcd: /* INTxx */
		/* fake a return stack frame and call real mode handler */
		trapno = *(kva + 1);
		sp = (u_int16_t *)((tf->tf_ss << 4) + tf->tf_esp);
		*(--sp) = tf->tf_eflags;
		*(--sp) = tf->tf_cs;
		*(--sp) = tf->tf_eip + 2;
		tf->tf_esp -= 6;
		tf->tf_cs = *(u_int16_t *)(trapno * 4 + 2);
		tf->tf_eip = *(u_int16_t *)(trapno * 4);
		break;
	case 0xcf: /* IRET */
		sp = (u_int16_t *)((tf->tf_ss << 4) + tf->tf_esp);
		tf->tf_eip = *(sp++);
		tf->tf_cs = *(sp++);
		tf->tf_eflags = *(sp++);
		tf->tf_esp += 6;
		tf->tf_eflags |= PSL_VM; /* outside of 16bit flag reg */
		break;
#ifndef KVM86_IOPL3 /* XXX check VME? */
	case 0xfa: /* CLI */
	case 0xfb: /* STI */
		/* XXX ignore for now */
		tf->tf_eip++;
		break;
	case 0x9c: /* PUSHF */
		sp = (u_int16_t *)((tf->tf_ss << 4) + tf->tf_esp);
		*(--sp) = tf->tf_eflags;
		tf->tf_esp -= 2;
		tf->tf_eip++;
		break;
	case 0x9d: /* POPF */
		sp = (u_int16_t *)((tf->tf_ss << 4) + tf->tf_esp);
		tf->tf_eflags = *(sp++);
		tf->tf_esp += 2;
		tf->tf_eip++;
		tf->tf_eflags |= PSL_VM; /* outside of 16bit flag reg */
		break;
#endif
	default:
#ifdef KVM86DEBUG
		printf("kvm86_gpfault: unhandled\n");
#else
		printf("kvm86_gpfault: cs=%x, eip=%x, insn=%x, eflags=%x\n",
		       tf->tf_cs, tf->tf_eip, insn, tf->tf_eflags);
#endif
		/*
		 * signal error to caller
		 */
		kvm86_ret(tf, -1);
		break;
	}
}
