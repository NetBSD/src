/*	$NetBSD: isadmavar.h,v 1.1 1998/06/08 17:49:44 tv Exp $	*/

#define	DMAMODE_WRITE	        0
#define	DMAMODE_READ	        1
#define	DMAMODE_LOOP            2

void isa_dmacascade __P((int));
void isa_dmastart __P((int, caddr_t, vm_size_t, int));
void isa_dmaabort __P((int));
int isa_dmafinished __P((int));
void isa_dmadone __P((int, caddr_t, vm_size_t, int));

#ifdef SHARK
caddr_t isa_dmabuffer_get __P(());
void isa_dmabuffer_finish __P(());
void	isa_dmafreeze __P((void));
void	isa_dmathaw __P((void));
#endif
