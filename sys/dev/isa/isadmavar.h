/*	$NetBSD: isadmavar.h,v 1.5 1997/03/21 00:00:22 mycroft Exp $	*/

#define	DMAMODE_WRITE	0
#define	DMAMODE_READ	1
#define	DMAMODE_LOOP	2

void isa_dmacascade __P((int));
void isa_dmastart __P((int, caddr_t, vm_size_t, int));
void isa_dmaabort __P((int));
vm_size_t isa_dmacount __P((int));
int isa_dmafinished __P((int));
void isa_dmadone __P((int, caddr_t, vm_size_t, int));
