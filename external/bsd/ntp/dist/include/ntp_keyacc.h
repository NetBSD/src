/*	$NetBSD: ntp_keyacc.h,v 1.1.1.1.6.2 2016/05/08 22:02:08 snj Exp $	*/

/*
 *  ntp_keyacc.h - key access stuff
 */
#ifndef NTP_KEYACC_H
#define NTP_KEYACC_H

typedef struct keyaccess KeyAccT;
struct keyaccess {
	KeyAccT *	next;
	sockaddr_u	addr;
};

extern KeyAccT* keyacc_new_push(KeyAccT *head, const sockaddr_u *addr);
extern KeyAccT* keyacc_pop_free(KeyAccT *head);
extern KeyAccT* keyacc_all_free(KeyAccT *head);
extern int      keyacc_contains(const KeyAccT *head, const sockaddr_u *addr,
				int res_on_empty_list);

#endif	/* NTP_KEYACC_H */
