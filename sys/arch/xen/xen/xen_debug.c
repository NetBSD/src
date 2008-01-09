/*	$NetBSD: xen_debug.c,v 1.4.50.1 2008/01/09 01:50:22 matt Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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

/*
 *
 * Copyright (c) 2002-2003, K A Fraser & R Neugebauer
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_debug.c,v 1.4.50.1 2008/01/09 01:50:22 matt Exp $");

#define XENDEBUG

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/stdarg.h>
#include <xen/xen.h>
#include <xen/hypervisor.h>

#ifdef XENDEBUG

#define PRINTK_BUFSIZE 1024
void
printk(const char *fmt, ...)
{
	va_list ap;
	int ret;
	static char buf[PRINTK_BUFSIZE];

	va_start(ap, fmt);
	ret = vsnprintf(buf, PRINTK_BUFSIZE - 1, fmt, ap);
	va_end(ap);
	buf[ret] = 0;
	(void)HYPERVISOR_console_io(CONSOLEIO_write, ret, buf);
}

void
vprintk(const char *fmt, va_list ap)
{
	int ret;
	static char buf[PRINTK_BUFSIZE];

	ret = vsnprintf(buf, PRINTK_BUFSIZE - 1, fmt, ap);
	buf[ret] = 0;
	(void)HYPERVISOR_console_io(CONSOLEIO_write, ret, buf);
}

#endif

#ifdef XENDEBUG_LOW

int xen_once = 0;

void hypervisor_callback(void);
void failsafe_callback(void);

void xen_dbglow_init(void);
void
xen_dbglow_init()
{
	start_info_t *si;
#if 0
	int i;
#endif

	si = &xen_start_info;

	HYPERVISOR_set_callbacks(
		__KERNEL_CS, (unsigned long)hypervisor_callback,
		__KERNEL_CS, (unsigned long)failsafe_callback);

	trap_init();

	/* __sti(); */

	/* print out some useful information  */
	printk(version);
	printk("start_info:   %p\n",  si);
	printk("  nr_pages:   %lu",   si->nr_pages);
	printk("  shared_inf: %p (was %p)\n",  HYPERVISOR_shared_info,
	    si->shared_info);
	printk("  pt_base:    %p",    (void *)si->pt_base); 
	printk("  mod_start:  0x%lx\n", si->mod_start);
	printk("  mod_len:    %lu\n", si->mod_len); 
#if 0
	printk("  net_rings: ");
	for (i = 0; i < MAX_DOMAIN_VIFS; i++) {
		if (si->net_rings[i] == 0)
			break;
		printk(" %lx", si->net_rings[i]);
	};
	printk("\n");
	printk("  blk_ring:   0x%lx\n", si->blk_ring);
#endif
	printk("  dom_id:     %d\n",  si->dom_id);
	printk("  flags:      0x%lx\n", si->flags);
	printk("  cmd_line:   %s\n",  si->cmd_line ?
	    (const char *)si->cmd_line : "NULL");
}


void xen_dbg0(char *);
void
xen_dbg0(char *end)
{
	struct cpu_info *ci;

	ci = &cpu_info_primary;
	if (xen_once)
	printk("xencpu level %d ipending %08x master %08x\n",
	    ci->ci_ilevel, ci->ci_ipending, 
	    HYPERVISOR_shared_info->events_mask);
	    /* ipending %08x imask %08x iunmask %08x */
	    /* ci->ci_imask[IPL_NET], ci->ci_iunmask[IPL_NET]); */
}

void xen_dbg1(void *esp, int ss);
void
xen_dbg1(void *esp, int ss)
{
#if 1
	struct cpu_info *ci;

	ci = &cpu_info_primary;
	if (xen_once)
	printk("xenhighlevel %d ipending %08x master %08x events %08x\n",
	    ci->ci_ilevel, ci->ci_ipending, 
	    HYPERVISOR_shared_info->events_mask, HYPERVISOR_shared_info->events);
#else
	printk("stack switch %p %d/%d, sp %p\n", esp, ss, IDXSEL(ss), &ss);
#endif
}

void xen_dbg2(void);
void
xen_dbg2(void)
{
	if (xen_once)
	printk("xen_dbg2\n");
}

void xen_dbg3(void *, void *);
void
xen_dbg3(void *ss, void *esp)
{
	if (xen_once)
	printk("xen_dbg3 %p %p\n", ss, esp);
}

void xen_dbg4(void *);
void
xen_dbg4(void *esi)
{

	printk("xen_dbg4 %p\n", esi);
	for(;;);
}




static void do_exit(void);

/*
 * These are assembler stubs in vector.S.
 * They are the actual entry points for virtual exceptions.
 */
void divide_error(void);
void debug(void);
void int3(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void double_fault(void);
void coprocessor_segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void page_fault(void);
void coprocessor_error(void);
void simd_coprocessor_error(void);
void alignment_check(void);
void spurious_interrupt_bug(void);
void machine_check(void);

static void
dump_regs(struct pt_regs *regs)
{
	int in_kernel = 1;
	unsigned long esp;
	unsigned short ss;

	esp = (unsigned long) (&regs->esp);
	ss = __KERNEL_DS;
	if (regs->xcs & 2) {
		in_kernel = 0;
		esp = regs->esp;
		ss = regs->xss & 0xffff;
	}
	printf("EIP:    %04x:[<%08lx>]\n",
	    0xffff & regs->xcs, regs->eip);
	printf("EFLAGS: %08lx\n",regs->eflags);
	printf("eax: %08lx   ebx: %08lx   ecx: %08lx   edx: %08lx\n",
	    regs->eax, regs->ebx, regs->ecx, regs->edx);
	printf("esi: %08lx   edi: %08lx   ebp: %08lx   esp: %08lx\n",
	    regs->esi, regs->edi, regs->ebp, esp);
	printf("ds: %04x   es: %04x   ss: %04x\n",
	    regs->xds & 0xffff, regs->xes & 0xffff, ss);
	printf("\n");
}	


static inline void
dump_code(unsigned eip)
{
	unsigned *ptr = (unsigned *)eip;
	int x;

	printk("Bytes at eip:\n");
	for (x = -4; x < 5; x++)
		printf("%x", ptr[x]);
}


/*
 * C handlers here have their parameter-list constructed by the
 * assembler stubs above. Each one gets a pointer to a list
 * of register values (to be restored at end of exception).
 * Some will also receive an error code -- this is the code that
 * was generated by the processor for the underlying real exception. 
 * 
 * Note that the page-fault exception is special. It also receives
 * the faulting linear address. Normally this would be found in
 * register CR2, but that is not accessible in a virtualised OS.
 */

static void inline
do_trap(int trapnr, char *str, struct pt_regs *regs, long error_code)
{

	printk("FATAL:  Unhandled Trap (see mini-os:traps.c)");
	printf("%d %s", trapnr, str);
	dump_regs(regs);
	dump_code(regs->eip);

	do_exit();
}

#define DO_ERROR(trapnr, str, name) \
void do_##name(struct pt_regs *regs, long error_code); \
void do_##name(struct pt_regs *regs, long error_code) \
{ \
	do_trap(trapnr, str, regs, error_code); \
}

#define DO_ERROR_INFO(trapnr, str, name, sicode, siaddr) \
void do_##name(struct pt_regs *regs, long error_code); \
void do_##name(struct pt_regs *regs, long error_code) \
{ \
	do_trap(trapnr, str, regs, error_code); \
}

DO_ERROR_INFO( 0, "divide error", divide_error, FPE_INTDIV, regs->eip)
DO_ERROR( 3, "int3", int3)
DO_ERROR( 4, "overflow", overflow)
DO_ERROR( 5, "bounds", bounds)
DO_ERROR_INFO( 6, "invalid operand", invalid_op, ILL_ILLOPN, regs->eip)
DO_ERROR( 7, "device not available", device_not_available)
DO_ERROR( 8, "double fault", double_fault)
DO_ERROR( 9, "coprocessor segment overrun", coprocessor_segment_overrun)
DO_ERROR(10, "invalid TSS", invalid_TSS)
DO_ERROR(11, "segment not present", segment_not_present)
DO_ERROR(12, "stack segment", stack_segment)
DO_ERROR_INFO(17, "alignment check", alignment_check, BUS_ADRALN, 0)
DO_ERROR(18, "machine check", machine_check)

void do_page_fault(struct pt_regs *, long, unsigned long);
void
do_page_fault(struct pt_regs *regs, long error_code, unsigned long address)
{

	printk("Page fault\n");
	printk("Address: 0x%lx", address);
	printk("Error Code: 0x%lx", error_code);
	printk("eip: \t 0x%lx", regs->eip);
	do_exit();
}

void do_general_protection(struct pt_regs *, long);
void
do_general_protection(struct pt_regs *regs, long error_code)
{

	HYPERVISOR_shared_info->events_mask = 0;
	printk("GPF\n");
	printk("Error Code: 0x%lx", error_code);
	dump_regs(regs);
	dump_code(regs->eip);
	do_exit();
}


void do_debug(struct pt_regs *, long);
void
do_debug(struct pt_regs *regs, long error_code)
{

	printk("Debug exception\n");
#define TF_MASK 0x100
	regs->eflags &= ~TF_MASK;
	dump_regs(regs);
	do_exit();
}



void do_coprocessor_error(struct pt_regs *, long);
void
do_coprocessor_error(struct pt_regs *regs, long error_code)
{

	printk("Copro error\n");
	dump_regs(regs);
	dump_code(regs->eip);
	do_exit();
}

void simd_math_error(void *);
void
simd_math_error(void *eip)
{

	printk("SIMD error\n");
}

void do_simd_coprocessor_error(struct pt_regs *, long);
void
do_simd_coprocessor_error(struct pt_regs *regs, long error_code)
{

	printk("SIMD copro error\n");
}

void do_spurious_interrupt_bug(struct pt_regs *, long);
void
do_spurious_interrupt_bug(struct pt_regs *regs, long error_code)
{
}

static void
do_exit(void)
{

	HYPERVISOR_exit();
}

/*
 * Submit a virtual IDT to teh hypervisor. This consists of tuples
 * (interrupt vector, privilege ring, CS:EIP of handler).
 * The 'privilege ring' field specifies the least-privileged ring that
 * can trap to that vector using a software-interrupt instruction (INT).
 */
static trap_info_t trap_table[] = {
    {  0, 0, __KERNEL_CS, (unsigned long)divide_error                },
    {  1, 0, __KERNEL_CS, (unsigned long)debug                       },
    {  3, 3, __KERNEL_CS, (unsigned long)int3                        },
    {  4, 3, __KERNEL_CS, (unsigned long)overflow                    },
    {  5, 3, __KERNEL_CS, (unsigned long)bounds                      },
    {  6, 0, __KERNEL_CS, (unsigned long)invalid_op                  },
    {  7, 0, __KERNEL_CS, (unsigned long)device_not_available        },
    {  8, 0, __KERNEL_CS, (unsigned long)double_fault                },
    {  9, 0, __KERNEL_CS, (unsigned long)coprocessor_segment_overrun },
    { 10, 0, __KERNEL_CS, (unsigned long)invalid_TSS                 },
    { 11, 0, __KERNEL_CS, (unsigned long)segment_not_present         },
    { 12, 0, __KERNEL_CS, (unsigned long)stack_segment               },
    { 13, 0, __KERNEL_CS, (unsigned long)general_protection          },
    { 14, 0, __KERNEL_CS, (unsigned long)page_fault                  },
    { 15, 0, __KERNEL_CS, (unsigned long)spurious_interrupt_bug      },
    { 16, 0, __KERNEL_CS, (unsigned long)coprocessor_error           },
    { 17, 0, __KERNEL_CS, (unsigned long)alignment_check             },
    { 18, 0, __KERNEL_CS, (unsigned long)machine_check               },
    { 19, 0, __KERNEL_CS, (unsigned long)simd_coprocessor_error      },
    {  0, 0,           0, 0                           }
};
    
void
trap_init(void)
{

	HYPERVISOR_set_trap_table(trap_table);    
}
#endif
