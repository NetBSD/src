/*	$NetBSD: isadmavar.h,v 1.3 1996/02/20 04:17:06 mycroft Exp $	*/

void isa_dmacascade __P((int));
void isa_dmastart __P((int, caddr_t, vm_size_t, int));
void isa_dmaabort __P((int));
int isa_dmafinished __P((int));
void isa_dmadone __P((int, caddr_t, vm_size_t, int));
