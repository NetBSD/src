#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/gdt.h>

#define	GDTSTART	64
#define	MAXGDTSIZ	8192

union descriptor *dynamic_gdt = gdt;
int gdt_size = NGDT;
int gdt_next = NGDT;
int gdt_count = NGDT;

int gdt_flags;
#define	GDT_LOCKED	0x1
#define	GDT_WANTED	0x2

static inline void
gdt_lock()
{

	while ((gdt_flags & GDT_LOCKED) != 0) {
		gdt_flags |= GDT_WANTED;
		tsleep(&gdt_flags, PZERO, "gdtlck", 0);
	}
	gdt_flags |= GDT_LOCKED;
}

static inline void
gdt_unlock()
{

	gdt_flags &= ~GDT_LOCKED;
	if ((gdt_flags & GDT_WANTED) != 0) {
		gdt_flags &= ~GDT_WANTED;
		wakeup(&gdt_flags);
	}
}

void
gdt_compact()
{
	struct proc *p;
	struct pcb *pcb;
	int slot = NGDT, oslot;

	for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
		pcb = &p->p_addr->u_pcb;
		oslot = IDXSEL(pcb->pcb_tss_sel);
		if (oslot >= gdt_count) {
			while (dynamic_gdt[slot].sd.sd_type != SDT_SYSNULL) {
				if (++slot >= gdt_count)
					panic("gdt_compact botch 1");
			}
			dynamic_gdt[slot] = dynamic_gdt[oslot];
			dynamic_gdt[oslot] = dynamic_gdt[GNULL_SEL];
			pcb->pcb_tss_sel = GSEL(slot, SEL_KPL);
		}
		oslot = IDXSEL(pcb->pcb_ldt_sel);
		if (oslot >= gdt_count) {
			while (dynamic_gdt[slot].sd.sd_type != SDT_SYSNULL) {
				if (++slot >= gdt_count)
					panic("gdt_compact botch 2");
			}
			dynamic_gdt[slot] = dynamic_gdt[oslot];
			dynamic_gdt[oslot] = dynamic_gdt[GNULL_SEL];
			pcb->pcb_ldt_sel = GSEL(slot, SEL_KPL);
		}
	}
	for (; slot < gdt_count; slot++)
		if (dynamic_gdt[slot].sd.sd_type == SDT_SYSNULL)
			panic("gdt_compact botch 3");
	for (slot = gdt_count; slot < gdt_size; slot++)
		if (dynamic_gdt[slot].sd.sd_type != SDT_SYSNULL)
			panic("gdt_compact botch 4");
	gdt_next = gdt_count;
}

void
gdt_resize(newsize)
	int newsize;
{
	size_t len, oldlen;
	union descriptor *new_gdt, *ogdt;
	struct region_descriptor region;

	ogdt = dynamic_gdt;
	len = newsize * sizeof(union descriptor);
	new_gdt = (union descriptor *)kmem_alloc(kernel_map, len);
	oldlen = gdt_size * sizeof(union descriptor);
	if (len > oldlen) {
		bcopy(ogdt, new_gdt, oldlen);
		bzero(new_gdt + gdt_size, len - oldlen);
	} else
		bcopy(ogdt, new_gdt, len);

	setregion(&region, new_gdt, len - 1);
	lgdt(&region);
	dynamic_gdt = new_gdt;
	gdt_size = newsize;

	if (ogdt != gdt)
		kmem_free(kernel_map, (vm_offset_t)ogdt, oldlen);
}

/*
 * Get an unused slot from the end of the GDT.  Grow the GDT if there aren't
 * any available slots.
 */
int
gdt_get_slot()
{
	int slot;

	gdt_lock();

	if (gdt_next >= gdt_size) {
		if (gdt_count >= gdt_size) {
			if (gdt_size >= MAXGDTSIZ)
				panic("gdt_get_slot botch");
			if (dynamic_gdt == gdt)
				gdt_resize(GDTSTART);
			else
				gdt_resize(gdt_size * 2);
		} else
			gdt_compact();
	}
	slot = gdt_next++;

	gdt_count++;
	gdt_unlock();
	return (slot);
}

void
gdt_put_slot(slot)
	int slot;
{

	gdt_lock();
	gdt_count--;

	dynamic_gdt[slot] = dynamic_gdt[GNULL_SEL];
	/* 
	 * shrink the GDT if we're using less than 1/4 of it.
	 * Shrinking at that point means we'll still have room for
	 * almost 2x as many processes as are now running without
	 * having to grow the GDT.
	 */
	if (gdt_size > GDTSTART && gdt_count < gdt_size / 4) {
		gdt_compact();
		gdt_resize(gdt_size / 2);
	}

	gdt_unlock();
}

void
tss_alloc(pcb)
	struct pcb *pcb;
{
	int slot;

	slot = gdt_get_slot();
	setsegment(&dynamic_gdt[slot].sd, &pcb->pcb_tss, sizeof(struct pcb) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	pcb->pcb_tss_sel = GSEL(slot, SEL_KPL);
}

void
tss_free(pcb)
	struct pcb *pcb;
{

	gdt_put_slot(IDXSEL(pcb->pcb_tss_sel));
}

void
ldt_alloc(pcb, ldt, len)
	struct pcb *pcb;
	union descriptor *ldt;
	size_t len;
{
	int slot;

	slot = gdt_get_slot();
	setsegment(&dynamic_gdt[slot].sd, ldt, len - 1, SDT_SYSLDT, SEL_KPL, 0,
	    0);
	pcb->pcb_ldt_sel = GSEL(slot, SEL_KPL);
}

void
ldt_free(pcb)
	struct pcb *pcb;
{

	gdt_put_slot(IDXSEL(pcb->pcb_ldt_sel));
}
