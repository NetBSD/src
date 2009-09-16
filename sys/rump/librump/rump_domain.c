/*	$NetBSD: rump_domain.c,v 1.1.2.2 2009/09/16 13:38:04 yamt Exp $	*/

/*
 * Force reference to __start_link_set_domains, so that the
 * binutils 2.19 linker does not lose the symbol due to it being
 * PROVIDEd in 2.19 as opposed to earlier versions where it was
 * defined.
 *
 * Note: look into this again when all platforms use 2.19
 */
extern void *__start_link_set_domains;
void *rump_start_domains = &__start_link_set_domains;
extern void *__stop_link_set_domains;
void *rump_stop_domains = &__stop_link_set_domains;
