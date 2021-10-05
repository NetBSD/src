/*	$NetBSD: res_private.h,v 1.3.48.1 2021/10/05 11:06:58 martin Exp $	*/

#ifndef res_private_h
#define res_private_h

struct __res_state_ext {
	union res_sockaddr_union nsaddrs[MAXNS];
	struct sort_list {
		int     af;
		union {
			struct in_addr  ina;
			struct in6_addr in6a;
		} addr, mask;
	} sort_list[MAXRESOLVSORT];
	char nsuffix[64];
	char nsuffix2[64];
	struct timespec res_conf_time;
	int kq, resfd;
	pid_t kqpid;
};

extern int res_ourserver_p(const res_state, const struct sockaddr *);
extern int __res_vinit(res_state, int);

#endif

/*! \file */
