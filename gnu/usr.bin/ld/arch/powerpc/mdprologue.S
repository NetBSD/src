/*	$NetBSD: mdprologue.S,v 1.2 1998/01/05 22:00:44 cgd Exp $	*/

/*
 * PowerPC run-time link editor entry points.
 */

	.text
	.globl	_binder, _binder_entry

/*
 *	_rtl(int version, struct crt_ldso *crtp)
 */
_rtl:					/* crt0 calls us here */
	ret

 /*
  * First call to a procedure generally comes through here for
  * binding.
  */
_binder_entry:
	ret
