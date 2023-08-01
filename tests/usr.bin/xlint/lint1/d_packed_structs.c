/*	$NetBSD: d_packed_structs.c,v 1.5 2023/08/01 19:52:15 rillig Exp $	*/
# 3 "d_packed_structs.c"

/* packed tests */

/* lint1-extra-flags: -X 351 */

struct in_addr {
	int x;
};
struct ip_timestamp {
	char ipt_code;
	char ipt_len;
	char ipt_ptr;
	unsigned int
	    ipt_flg: 4,
	    ipt_oflw: 4;
	union ipt_timestamp {
		int ipt_time[1];
		struct ipt_ta {
			struct in_addr ipt_addr;
			int ipt_time;
		} ipt_ta[1]__packed;
	} ipt_timestamp__packed;
} __packed;

typedef struct __packed {
	int x;
} t;

struct x {
	char c;
	long l;
} __packed;

struct y {
	char c;
	long l;
};

int a[sizeof(struct y) - sizeof(struct x) - 1];

/* expect+1: error: negative array dimension (-9) [20] */
typedef int sizeof_x[-(int)sizeof(struct x)];
/* expect+1: error: negative array dimension (-16) [20] */
typedef int sizeof_y[-(int)sizeof(struct y)];
