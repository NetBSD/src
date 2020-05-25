/*	$NetBSD: ntp_psl.h,v 1.2 2020/05/25 20:47:19 christos Exp $	*/

#ifndef NTP_PSL_H
#define NTP_PSL_H


/*
 * Poll Skew List Item
 */

typedef struct psl_item_tag {
	int		sub;	/* int or short?  unsigned is OK, but why? */
	int		qty;	/* int or short?  unsigned is OK, but why? */
	int		msk;	/* int or short?  unsigned is OK */
} psl_item;

int get_pollskew(int, psl_item *);

#endif	/* !defined(NTP_PSL_H) */
