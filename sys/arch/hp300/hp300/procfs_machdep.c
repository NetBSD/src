#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <miscfs/procfs/procfs.h>

extern char kstack[];		/* XXX */

int
procfs_sstep(p)
	struct proc *p;
{
	struct frame *frame;

	if ((p->p_flag & SLOAD) == 0)
		return EIO;

	frame = (struct frame *)
	    ((char *)p->p_addr + ((char *)p->p_regs - (char *)kstack));

	frame->f_sr |= PSL_T;

	return 0;
}

int
procfs_fix_sstep(p)
	struct proc *p;
{

	return 0;
}

int
procfs_read_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct frame *frame;

	if ((p->p_flag & SLOAD) == 0)
		return EIO;

	frame = (struct frame *)
	    ((char *)p->p_addr + ((char *)p->p_regs - (char *)kstack));

	bcopy(frame->f_regs, regs->r_regs, sizeof(frame->f_regs));
	regs->r_sr = frame->f_sr;
	regs->r_pc = frame->f_pc;

	return 0;
}

int
procfs_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct frame *frame;

	if ((p->p_flag & SLOAD) == 0)
		return EIO;

	frame = (struct frame *)
	    ((char *)p->p_addr + ((char *)p->p_regs - (char *)kstack));

	if ((regs->r_sr & PSL_USERCLR) != 0 ||
	    (regs->r_sr & PSL_USERSET) != PSL_USERSET)
		return EPERM;

	bcopy(regs->r_regs, frame->f_regs, sizeof(frame->f_regs));
	frame->f_sr = regs->r_sr;
	frame->f_pc = regs->r_pc;

	return 0;
}
