#ifndef _ARM_PIC31_PICVAR_H_
#define _ARM_PIC31_PICVAR_H_

struct pic_softc;
struct intrsource;

int	pic_handle_intr(void *);
void	pic_mark_pending(struct pic_softc *pic, int irq);
void	pic_mark_pending_source(struct pic_softc *pic, struct intrsource *is);
void	*pic_establish_intr(struct pic_softc *pic, int irq, int ipl, int type,
	    int (*func)(void *), void *arg);
int	pic_alloc_irq(struct pic_softc *pic);
void	pic_disestablish_source(struct intrsource *is);
void	pic_do_pending_ints(register_t psw, int newipl);

#ifdef _INTR_PRIVATE

#ifndef PIC_MAXPICS
#define PIC_MAXPICS	32
#endif
#ifndef PIC_MAXSOURCES
#define	PIC_MAXSOURCES	64
#endif

struct intrsource {
	int (*is_func)(void *);
	void *is_arg;
	struct pic_softc *is_pic;		/* owning PIC */
	uint8_t is_type;			/* IST_xxx */
	uint8_t is_ipl;				/* IPL_xxx */
	uint8_t is_irq;				/* local to pic */
	struct evcnt is_ev;
	char is_source[16];
};

struct pic_softc {
	const struct pic_ops *pic_ops;
	struct intrsource **pic_sources;
	uint32_t pic_pending_irqs[(PIC_MAXSOURCES + 31) / 32];
	uint32_t pic_pending_ipls;
	size_t pic_maxsources;
	uint8_t pic_id;
	uint16_t pic_irqbase;
	char pic_name[14];
};

struct pic_ops {
	void (*pic_unblock_irqs)(struct pic_softc *, size_t, uint32_t);
	void (*pic_block_irqs)(struct pic_softc *, size_t, uint32_t);
	int (*pic_find_pending_irqs)(struct pic_softc *);

	void (*pic_establish_irq)(struct pic_softc *, int, int, int);
	void (*pic_source_name)(struct pic_softc *, int, char *, size_t);
};


bool	pic_add(struct pic_softc *, int);
void	pic_do_pending_int(void);

extern struct pic_softc * pic_list[PIC_MAXPICS];
extern volatile uint32_t pic_pending_pics[(PIC_MAXPICS + 31) / 32];
extern volatile uint32_t pic_pending_ipls;
extern int pic_pending_ipl_refcnt[NIPL];

#endif /* _INTR_PRIVATE */

#endif /* _ARM_PIC_PICVAR_H_ */
