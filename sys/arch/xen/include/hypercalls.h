/*	$NetBSD: hypercalls.h,v 1.2.12.1 2008/01/05 19:32:18 bouyer Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
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
 *
 */

/*
 * 
 * Communication to/from hypervisor.
 * 
 * Copyright (c) 2002-2004, K A Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


#ifndef _XENI386_HYPERVISOR_H_
#define _XENI386_HYPERVISOR_H_
/*
 * Assembler stubs for hyper-calls.
 */

#ifndef XEN3
#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12 /* XXX from i386/vmparam.h */
#else
#if PAGE_SHIFT != 12
#error "PAGE_SHIFT is wrong"
#endif
#endif
#endif /* !XEN3 */

#if defined(XEN3) && !defined(XEN_COMPAT_030001)
/* hypercall via the hypercall call page */
#define __str(x) #x
#define _str(x) __str(x)
#define _hypercall(name, input_const, output_const) \
	__asm volatile ( \
	    "call hypercall_page + ("_str(name)" * 32)" \
	    : output_const \
	    : input_const \
	    : "memory" )
#else 
/* traditionnal hypercall via int 0x82 */
#define _hypercall(name, input_const, output_const) \
	__asm volatile ( \
	    TRAP_INSTR \
	    : output_const \
	    : "0" (name), input_const \
	    : "memory" )
#endif

#define _harg(...) __VA_ARGS__
	

static __inline int
HYPERVISOR_set_trap_table(trap_info_t *table)
{
    int ret;
    unsigned long ign1;

	_hypercall(__HYPERVISOR_set_trap_table, _harg("1" (table)),
	    _harg("=a" (ret), "=b" (ign1)));

    return ret;
}

static __inline int
HYPERVISOR_set_gdt(unsigned long *frame_list, int entries)
{
    int ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_set_gdt, _harg("1" (frame_list), "2" (entries)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline int
HYPERVISOR_stack_switch(unsigned long ss, unsigned long esp)
{
    int ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_stack_switch, _harg("1" (ss), "2" (esp)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline int
HYPERVISOR_set_callbacks(
    unsigned long event_selector, unsigned long event_address,
    unsigned long failsafe_selector, unsigned long failsafe_address)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4;

    _hypercall(__HYPERVISOR_set_callbacks,
	_harg("1" (event_selector),"2" (event_address),
	    "3" (failsafe_selector), "4" (failsafe_address)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4)));

    return ret;
}

static __inline int
HYPERVISOR_dom0_op(dom0_op_t *dom0_op)
{
    int ret;
    unsigned long ign1;

    dom0_op->interface_version = DOM0_INTERFACE_VERSION;
    _hypercall(__HYPERVISOR_dom0_op, _harg("1" (dom0_op)),
	_harg("=a" (ret), "=b" (ign1)));

    return ret;
}

static __inline int
HYPERVISOR_set_debugreg(int reg, unsigned long value)
{
    int ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_set_debugreg, _harg("1" (reg), "2" (value)),
	 _harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline unsigned long
HYPERVISOR_get_debugreg(int reg)
{
    unsigned long ret;
    unsigned long ign1;

    _hypercall(__HYPERVISOR_get_debugreg, _harg("1" (reg)),
	_harg("=a" (ret), "=b" (ign1)));

    return ret;
}

#ifdef XEN3
static __inline int
HYPERVISOR_mmu_update(mmu_update_t *req, int count, int *success_count,
    domid_t domid)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4;

    _hypercall(__HYPERVISOR_mmu_update,
	_harg("1" (req), "2" (count), "3" (success_count), "4" (domid)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4)));

    return ret;
}

static __inline int
HYPERVISOR_mmuext_op(struct mmuext_op *op, int count, int *success_count,
    domid_t domid)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4;

    _hypercall(__HYPERVISOR_mmuext_op,
	_harg("1" (op), "2" (count), "3" (success_count), "4" (domid)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4)));

    return ret;
}

#if 0
static __inline int
HYPERVISOR_fpu_taskswitch(int set)
{
    long ret;
    long ign1;

    _hypercall(__HYPERVISOR_fpu_taskswitch, _harg("1" (set)),
	_harg("=a" (ret), "=b" (ign1)));

    return ret;
}
#else /* 0 */
/* Xen2 compat: always i38HYPERVISOR_fpu_taskswitch(1) */
static __inline int
HYPERVISOR_fpu_taskswitch(void)
{
    long ret;
    long ign1;
    _hypercall(__HYPERVISOR_fpu_taskswitch, _harg("1" (1)),
	_harg("=a" (ret), "=b" (ign1)));

    return ret;
}
#endif /* 0 */

static __inline int
HYPERVISOR_update_descriptor(uint64_t ma, uint32_t word1, uint32_t word2)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4;
    int ma1 = ma & 0xffffffff;
    int ma2 = (ma >> 32) & 0xffffffff;

    _hypercall(__HYPERVISOR_update_descriptor,
	_harg("1" (ma1), "2" (ma2), "3" (word1), "4" (word2)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4)));

    return ret;
}

static __inline int
HYPERVISOR_memory_op(unsigned int cmd, void *arg)
{
    int ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_memory_op, _harg("1" (cmd), "2" (arg)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline int
HYPERVISOR_update_va_mapping(unsigned long page_nr, unsigned long new_val,
    unsigned long flags)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4;

    _hypercall(__HYPERVISOR_update_va_mapping,
	_harg("1" (page_nr), "2" (new_val), "3" (0), "4" (flags)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4)));

#ifdef notdef
    if (__predict_false(ret < 0))
        panic("Failed update VA mapping: %08lx, %08lx, %08lx",
              page_nr, new_val, flags);
#endif

    return ret;
}

static __inline int
HYPERVISOR_xen_version(int cmd, void *arg)
{
    int ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_xen_version, _harg("1" (cmd), "2" (arg)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline int
HYPERVISOR_grant_table_op(unsigned int cmd, void *uop, unsigned int count)
{
    int ret;
    unsigned long ign1, ign2, ign3;

    _hypercall(__HYPERVISOR_grant_table_op,
	_harg("1" (cmd), "2" (uop), "3" (count)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)));

    return ret;
}

static __inline int
HYPERVISOR_update_va_mapping_otherdomain(unsigned long page_nr,
    unsigned long new_val, unsigned long flags, domid_t domid)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4, ign5;

    _hypercall(__HYPERVISOR_update_va_mapping_otherdomain,
	_harg("1" (page_nr), "2" (new_val), "3" (0), "4" (flags), "5" (domid)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4),
	    "=D" (ign5)));

    return ret;
}

static __inline int
HYPERVISOR_vcpu_op(int cmd, int vcpuid, void *extra_args)
{
    long ret;
    unsigned long ign1, ign2, ign3;

    _hypercall(__HYPERVISOR_vcpu_op,
	_harg("1" (cmd), "2" (vcpuid), "3" (extra_args)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)));

    return ret;
}

static __inline long
HYPERVISOR_yield(void)
{
    long ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_sched_op, _harg("1" (SCHEDOP_yield), "2" (0)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline long
HYPERVISOR_block(void)
{
    long ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_sched_op, _harg("1" (SCHEDOP_block), "2" (0)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline long
HYPERVISOR_shutdown(void)
{
    long ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_sched_op,
	_harg("1" (SCHEDOP_shutdown), "2" (SHUTDOWN_poweroff)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline long
HYPERVISOR_reboot(void)
{
    long ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_sched_op,
	_harg("1" (SCHEDOP_shutdown), "2" (SHUTDOWN_reboot)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}

static __inline long
HYPERVISOR_suspend(unsigned long srec)
{
    long ret;
    unsigned long ign1, ign2, ign3;

    _hypercall(__HYPERVISOR_sched_op,
	_harg("1" (SCHEDOP_shutdown), "2" (SHUTDOWN_suspend), "3" (srec)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)));

    return ret;
}

static __inline long
HYPERVISOR_set_timer_op(uint64_t timeout)
{
    long ret;
    unsigned long timeout_hi = (unsigned long)(timeout>>32);
    unsigned long timeout_lo = (unsigned long)timeout;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_set_timer_op,
	 _harg("1" (timeout_lo), "2" (timeout_hi)),
	 _harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}
#else /* !XEN3 */
static __inline int
HYPERVISOR_mmu_update(mmu_update_t *req, int count, int *success_count)
{
    int ret;
    unsigned long ign1, ign2, ign3;

    __asm__ __volatile__ (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)
	: "0" (__HYPERVISOR_mmu_update), "1" (req), "2" (count),
	  "3" (success_count)
	: "memory" );

    return ret;
}

static __inline int
HYPERVISOR_fpu_taskswitch(void)
{
    int ret;
    __asm volatile (
        TRAP_INSTR
        : "=a" (ret) : "0" (__HYPERVISOR_fpu_taskswitch) : "memory" );

    return ret;
}

static __inline int
HYPERVISOR_update_descriptor(unsigned long pa, unsigned long word1,
    unsigned long word2)
{
    int ret;
    unsigned long ign1, ign2, ign3;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)
	: "0" (__HYPERVISOR_update_descriptor), "1" (pa), "2" (word1),
	  "3" (word2)
	: "memory" );

    return ret;
}

static __inline int
HYPERVISOR_yield(void)
{
    int ret;
    unsigned long ign1;

    __asm__ __volatile__ (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1)
	: "0" (__HYPERVISOR_sched_op), "1" (SCHEDOP_yield)
	: "memory" );

    return ret;
}

static __inline int
HYPERVISOR_block(void)
{
    int ret;
    unsigned long ign1;

    __asm__ __volatile__ (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1)
	: "0" (__HYPERVISOR_sched_op), "1" (SCHEDOP_block)
	: "memory" );

    return ret;
}

static __inline int
HYPERVISOR_shutdown(void)
{
    int ret;
    unsigned long ign1;

    __asm__ __volatile__ (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1)
	: "0" (__HYPERVISOR_sched_op),
	  "1" (SCHEDOP_shutdown | (SHUTDOWN_poweroff << SCHEDOP_reasonshift))
        : "memory" );

    return ret;
}

static __inline int
HYPERVISOR_reboot(void)
{
    int ret;
    unsigned long ign1;

    __asm__ __volatile__ (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1)
	: "0" (__HYPERVISOR_sched_op),
	  "1" (SCHEDOP_shutdown | (SHUTDOWN_reboot << SCHEDOP_reasonshift))
        : "memory" );

    return ret;
}

static __inline int
HYPERVISOR_suspend(unsigned long srec)
{
    int ret;
    unsigned long ign1, ign2;

    /* NB. On suspend, control software expects a suspend record in %esi. */
    __asm__ __volatile__ (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=S" (ign2)
	: "0" (__HYPERVISOR_sched_op),
        "b" (SCHEDOP_shutdown | (SHUTDOWN_suspend << SCHEDOP_reasonshift)), 
        "S" (srec) : "memory");

    return ret;
}

static __inline int
HYPERVISOR_set_fast_trap(int idx)
{
    int ret;
    unsigned long ign1;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1)
	: "0" (__HYPERVISOR_set_fast_trap), "1" (idx)
	: "memory" );

    return ret;
}

static __inline int
HYPERVISOR_dom_mem_op(unsigned int op, unsigned long *extent_list,
    unsigned long nr_extents, unsigned int extent_order)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4, ign5;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4),
	  "=D" (ign5)
	: "0" (__HYPERVISOR_dom_mem_op), "1" (op), "2" (extent_list),
	  "3" (nr_extents), "4" (extent_order), "5" (DOMID_SELF)
        : "memory" );

    return ret;
}

static __inline int
HYPERVISOR_update_va_mapping(unsigned long va, unsigned long new_val,
    unsigned long flags)
{
    int ret;
    unsigned long ign1, ign2, ign3;
    unsigned long page_nr = va >> PAGE_SHIFT;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)
	: "0" (__HYPERVISOR_update_va_mapping), 
          "1" (page_nr), "2" (new_val), "3" (flags)
	: "memory" );

#ifdef notdef
    if (__predict_false(ret < 0))
        panic("Failed update VA mapping: %08lx, %08lx, %08lx",
              page_nr, new_val, flags);
#endif

    return ret;
}

static __inline int
HYPERVISOR_xen_version(int cmd)
{
    int ret;
    unsigned long ign1;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1)
	: "0" (__HYPERVISOR_xen_version), "1" (cmd)
	: "memory" );

    return ret;
}

static __inline int
HYPERVISOR_grant_table_op(unsigned int cmd, void *uop, unsigned int count)
{
    int ret;
    unsigned long ign1, ign2, ign3;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)
	: "0" (__HYPERVISOR_grant_table_op), "1" (cmd), "2" (count), "3" (uop)
	: "memory" );

    return ret;
}

static __inline int
HYPERVISOR_update_va_mapping_otherdomain(unsigned long va,
    unsigned long new_val, unsigned long flags, domid_t domid)
{
    int ret;
    unsigned long ign1, ign2, ign3, ign4;
    unsigned long page_nr = va >> PAGE_SHIFT;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3), "=S" (ign4)
	: "0" (__HYPERVISOR_update_va_mapping_otherdomain),
          "1" (page_nr), "2" (new_val), "3" (flags), "4" (domid) :
        "memory" );
    
    return ret;
}

static __inline long
HYPERVISOR_set_timer_op(uint64_t timeout)
{
    long ret;
    unsigned long timeout_hi = (unsigned long)(timeout>>32);
    unsigned long timeout_lo = (unsigned long)timeout;
    unsigned long ign1, ign2;

    __asm volatile (
        TRAP_INSTR
        : "=a" (ret), "=b" (ign1), "=c" (ign2)
	: "0" (__HYPERVISOR_set_timer_op), "b" (timeout_hi), "c" (timeout_lo)
	: "memory");

    return ret;
}
#endif /* XEN3 */

static __inline int
HYPERVISOR_multicall(void *call_list, int nr_calls)
{
    int ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_multicall,
	 _harg("1" (call_list), "2" (nr_calls)),
	 _harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}


static __inline int
HYPERVISOR_event_channel_op(void *op)
{
    int ret;
    unsigned long ign1;

    _hypercall(__HYPERVISOR_event_channel_op, _harg("1" (op)),
	_harg("=a" (ret), "=b" (ign1)));

    return ret;
}

static __inline int
HYPERVISOR_console_io(int cmd, int count, char *str)
{
    int ret;
    unsigned long ign1, ign2, ign3;

    _hypercall(__HYPERVISOR_console_io,
	_harg("1" (cmd), "2" (count), "3" (str)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2), "=d" (ign3)));

    return ret;
}

static __inline int
HYPERVISOR_physdev_op(void *physdev_op)
{
    int ret;
    unsigned long ign1;

    _hypercall(__HYPERVISOR_physdev_op, _harg("1" (physdev_op)),
	_harg("=a" (ret), "=b" (ign1)));

    return ret;
}

static __inline int
HYPERVISOR_vm_assist(unsigned int cmd, unsigned int type)
{
    int ret;
    unsigned long ign1, ign2;

    _hypercall(__HYPERVISOR_vm_assist, _harg("1" (cmd), "2" (type)),
	_harg("=a" (ret), "=b" (ign1), "=c" (ign2)));

    return ret;
}
#endif /* _XENI386_HYPERVISOR_H_ */
