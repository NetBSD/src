/*	$NetBSD: rump_module.c,v 1.1 2009/09/13 22:51:42 pooka Exp $	*/

/*
 * Force reference to __start_link_set_modules, so that the
 * binutils 2.19 linker does not lose the symbol due to it being
 * PROVIDEd in 2.19 as opposed to earlier versions where it was
 * defined.
 *
 * Note: look into this again when all platforms use 2.19
 */
extern void *__start_link_set_modules;
void *rump_start_modules = &__start_link_set_modules;
extern void *__stop_link_set_modules;
void *rump_stop_modules = &__stop_link_set_modules;
