/* $NetBSD: vmtvar.h,v 1.2 2021/03/27 21:23:14 ryo Exp $ */
/* NetBSD: vmt.c,v 1.15 2016/11/10 03:32:04 ozaki-r Exp */
/* $OpenBSD: vmt.c,v 1.11 2011/01/27 21:29:25 dtucker Exp $ */

/*
 * Copyright (c) 2007 David Crawshaw <david@zentus.com>
 * Copyright (c) 2008 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DEV_VMT_VMTVAR_H_
#define _DEV_VMT_VMTVAR_H_

#include <sys/uuid.h>
#include <dev/sysmon/sysmonvar.h>

/* A register frame. */
/* XXX 'volatile' as a workaround because BACKDOOR_OP is likely broken */
struct vm_backdoor {
	volatile register_t eax;
	volatile register_t ebx;
	volatile register_t ecx;
	volatile register_t edx;
	volatile register_t esi;
	volatile register_t edi;
	volatile register_t ebp;
};

#define VM_REG_LOW_MASK		__BITS(15,0)
#define VM_REG_HIGH_MASK	__BITS(31,16)
#define VM_REG_WORD_MASK	__BITS(31,0)
#define VM_REG_CMD(hi, low)	\
	(__SHIFTIN((hi), VM_REG_HIGH_MASK) | __SHIFTIN((low), VM_REG_LOW_MASK))
#define VM_REG_CMD_RPC(cmd)	VM_REG_CMD((cmd), VM_CMD_RPC)
#define VM_REG_PORT_CMD(cmd)	VM_REG_CMD((cmd), VM_PORT_CMD)
#define VM_REG_PORT_RPC(cmd)	VM_REG_CMD((cmd), VM_PORT_RPC)

/* RPC context. */
struct vm_rpc {
	uint16_t channel;
	uint32_t cookie1;
	uint32_t cookie2;
};

struct vmt_event {
	struct sysmon_pswitch	ev_smpsw;
	int			ev_code;
};

struct vmt_softc {
	device_t		sc_dev;

	struct sysctllog	*sc_log;
	struct vm_rpc		sc_tclo_rpc;
	bool			sc_tclo_rpc_open;
	char			*sc_rpc_buf;
	int			sc_rpc_error;
	int			sc_tclo_ping;
	int			sc_set_guest_os;
#define VMT_RPC_BUFLEN			256

	struct callout		sc_tick;
	struct callout		sc_tclo_tick;

#define VMT_CLOCK_SYNC_PERIOD_SECONDS 60
	int			sc_clock_sync_period_seconds;
	struct callout		sc_clock_sync_tick;

	struct vmt_event	sc_ev_power;
	struct vmt_event	sc_ev_reset;
	struct vmt_event	sc_ev_sleep;
	bool			sc_smpsw_valid;

	char			sc_hostname[MAXHOSTNAMELEN];
	char			sc_uuid[_UUID_STR_LEN];
};

bool vmt_probe(void);
void vmt_common_attach(struct vmt_softc *);
int vmt_common_detach(struct vmt_softc *);

#define BACKDOOR_OP_I386(op, frame)		\
	__asm__ __volatile__ (			\
		"pushal;"			\
		"pushl %%eax;"			\
		"movl 0x18(%%eax), %%ebp;"	\
		"movl 0x14(%%eax), %%edi;"	\
		"movl 0x10(%%eax), %%esi;"	\
		"movl 0x0c(%%eax), %%edx;"	\
		"movl 0x08(%%eax), %%ecx;"	\
		"movl 0x04(%%eax), %%ebx;"	\
		"movl 0x00(%%eax), %%eax;"	\
		op				\
		"xchgl %%eax, 0x00(%%esp);"	\
		"movl %%ebp, 0x18(%%eax);"	\
		"movl %%edi, 0x14(%%eax);"	\
		"movl %%esi, 0x10(%%eax);"	\
		"movl %%edx, 0x0c(%%eax);"	\
		"movl %%ecx, 0x08(%%eax);"	\
		"movl %%ebx, 0x04(%%eax);"	\
		"popl 0x00(%%eax);"		\
		"popal;"			\
		:				\
		:"a"(frame)			\
	)

#define BACKDOOR_OP_AMD64(op, frame)		\
	__asm__ __volatile__ (			\
		"pushq %%rbp;			\n\t" \
		"pushq %%rax;			\n\t" \
		"movq 0x30(%%rax), %%rbp;	\n\t" \
		"movq 0x28(%%rax), %%rdi;	\n\t" \
		"movq 0x20(%%rax), %%rsi;	\n\t" \
		"movq 0x18(%%rax), %%rdx;	\n\t" \
		"movq 0x10(%%rax), %%rcx;	\n\t" \
		"movq 0x08(%%rax), %%rbx;	\n\t" \
		"movq 0x00(%%rax), %%rax;	\n\t" \
		op				"\n\t" \
		"xchgq %%rax, 0x00(%%rsp);	\n\t" \
		"movq %%rbp, 0x30(%%rax);	\n\t" \
		"movq %%rdi, 0x28(%%rax);	\n\t" \
		"movq %%rsi, 0x20(%%rax);	\n\t" \
		"movq %%rdx, 0x18(%%rax);	\n\t" \
		"movq %%rcx, 0x10(%%rax);	\n\t" \
		"movq %%rbx, 0x08(%%rax);	\n\t" \
		"popq 0x00(%%rax);		\n\t" \
		"popq %%rbp;			\n\t" \
		: /* No outputs. */ \
		: "a" (frame) \
		  /* No pushal on amd64 so warn gcc about the clobbered registers. */\
		: "rbx", "rcx", "rdx", "rdi", "rsi", "cc", "memory" \
	)

#define X86_IO_MAGIC		0x86	/* magic for upper 32bit of x7 */
#define X86_IO_W7_SIZE_MASK	__BITS(1, 0)
#define X86_IO_W7_SIZE(n)	__SHIFTIN((n), X86_IO_W7_SIZE_MASK)
#define X86_IO_W7_DIR		__BIT(2)
#define X86_IO_W7_WITH		__BIT(3)
#define X86_IO_W7_STR		__BIT(4)
#define X86_IO_W7_DF		__BIT(5)
#define X86_IO_W7_IMM_MASK	__BITS(12, 5)
#define X86_IO_W7_IMM(imm)	__SHIFTIN((imm), X86_IO_W7_IMM_MASK)
#define BACKDOOR_OP_AARCH64(op, frame)		\
	__asm__ __volatile__ (			\
		"ldp x0, x1, [%0, 8 * 0];	\n\t" \
		"ldp x2, x3, [%0, 8 * 2];	\n\t" \
		"ldp x4, x5, [%0, 8 * 4];	\n\t" \
		"ldr x6,     [%0, 8 * 6];	\n\t" \
		"mov x7, %1			\n\t" \
		"movk x7, %2, lsl #32;		\n\t" \
		"mrs xzr, mdccsr_el0;		\n\t" \
		"stp x0, x1, [%0, 8 * 0];	\n\t" \
		"stp x2, x3, [%0, 8 * 2];	\n\t" \
		"stp x4, x5, [%0, 8 * 4];	\n\t" \
		"str x6,     [%0, 8 * 6];	\n\t" \
		: /* No outputs. */ \
		: "r" (frame), \
		  "r" (op), \
		  "i" (X86_IO_MAGIC) \
		: "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "memory" \
	)

#if defined(__i386__)
#define BACKDOOR_OP(op, frame) BACKDOOR_OP_I386(op, frame)
#elif defined(__amd64__)
#define BACKDOOR_OP(op, frame) BACKDOOR_OP_AMD64(op, frame)
#elif defined(__aarch64__)
#define BACKDOOR_OP(op, frame) BACKDOOR_OP_AARCH64(op, frame)
#endif

#if defined(__i386__) || defined(__amd64__)
#define BACKDOOR_OP_CMD	"inl %%dx, %%eax;"
#define BACKDOOR_OP_IN	"cld;\n\trep insb;"
#define BACKDOOR_OP_OUT	"cld;\n\trep outsb;"
#elif defined(__aarch64__)
#define BACKDOOR_OP_CMD	(X86_IO_W7_WITH | X86_IO_W7_DIR | X86_IO_W7_SIZE(2))
#define BACKDOOR_OP_IN	(X86_IO_W7_WITH | X86_IO_W7_STR | X86_IO_W7_DIR)
#define BACKDOOR_OP_OUT	(X86_IO_W7_WITH | X86_IO_W7_STR)
#endif

static __inline void
vmt_hvcall(uint8_t cmd, u_int regs[6])
{
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));
	frame.eax = VM_MAGIC;
	frame.ebx = UINT_MAX;
	frame.ecx = VM_REG_CMD(0, cmd);
	frame.edx = VM_REG_PORT_CMD(0);

	BACKDOOR_OP(BACKDOOR_OP_CMD, &frame);

	regs[0] = __SHIFTOUT(frame.eax, VM_REG_WORD_MASK);
	regs[1] = __SHIFTOUT(frame.ebx, VM_REG_WORD_MASK);
	regs[2] = __SHIFTOUT(frame.ecx, VM_REG_WORD_MASK);
	regs[3] = __SHIFTOUT(frame.edx, VM_REG_WORD_MASK);
	regs[4] = __SHIFTOUT(frame.esi, VM_REG_WORD_MASK);
	regs[5] = __SHIFTOUT(frame.edi, VM_REG_WORD_MASK);
}

#endif /* _DEV_VMT_VMTVAR_H_ */
