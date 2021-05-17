/*	$NetBSD: if_lagg.h,v 1.1 2021/05/17 04:07:43 yamaguchi Exp $	*/

#ifndef _NET_LAGG_IF_LAGG_H_
#define _NET_LAGG_IF_LAGG_H_

typedef enum {
	LAGG_PROTO_NONE = 0,
	LAGG_PROTO_LACP,	/* 802.1ax lacp */
	LAGG_PROTO_FAILOVER,
	LAGG_PROTO_LOADBALANCE,
	LAGG_PROTO_MAX,
} lagg_proto;

/* IEEE802.3ad LACP protocol definitions.*/
#define LACP_STATE_ACTIVITY	__BIT(0)
#define LACP_STATE_TIMEOUT	__BIT(1)
#define LACP_STATE_AGGREGATION	__BIT(2)
#define LACP_STATE_SYNC		__BIT(3)
#define LACP_STATE_COLLECTING	__BIT(4)
#define LACP_STATE_DISTRIBUTING	__BIT(5)
#define LACP_STATE_DEFAULTED	__BIT(6)
#define LACP_STATE_EXPIRED	__BIT(7)
#define LACP_STATE_BITS		\
	"\020"			\
	"\001ACTIVITY"		\
	"\002TIMEOUT"		\
	"\003AGGREGATION"	\
	"\004SYNC"		\
	"\005COLLECTING"	\
	"\006DISTRIBUTING"	\
	"\007DEFAULTED"		\
	"\010EXPIRED"
#define LACP_STATESTR_LEN	256
#define LACP_MAC_LEN		ETHER_ADDR_LEN

enum lagg_ioctl_lacp {
	LAGGIOC_LACPSETFLAGS = 1,
	LAGGIOC_LACPCLRFLAGS,
	LAGGIOC_LACPSETMAXPORTS,
	LAGGIOC_LACPCLRMAXPORTS,
};

#define LAGGREQLACP_OPTIMISTIC		__BIT(0)
#define LAGGREQLACP_DUMPDU		__BIT(1)
#define LAGGREQLACP_STOPDU		__BIT(2)
#define LAGGREQLACP_MULTILS		__BIT(3)
#define LAGGREQLACP_BITS		\
	"\020"				\
	"\001OPTIMISTIC"		\
	"\002DUMPDU"			\
	"\003STOPDU"			\
	"\004MULTILS"

struct laggreq_lacp {
	uint32_t	 command;
	uint32_t	 flags;
	size_t		 maxports;

	uint16_t	 actor_prio;
	uint8_t		 actor_mac[LACP_MAC_LEN];
	uint16_t	 actor_key;
	uint16_t	 partner_prio;
	uint8_t		 partner_mac[LACP_MAC_LEN];
	uint16_t	 partner_key;
};

enum lagg_ioctl_fail {
	LAGGIOC_FAILSETFLAGS = 1,
	LAGGIOC_FAILCLRFLAGS
};

#define LAGGREQFAIL_RXALL		__BIT(0)

struct laggreq_fail {
	uint32_t	 command;
	uint32_t	 flags;
};

struct laggreqproto {
	union {
		struct laggreq_lacp	 proto_lacp;
		struct laggreq_fail	 proto_fail;
	} rp_proto;
#define rp_lacp	rp_proto.proto_lacp
#define rp_fail	rp_proto.proto_fail
};

#define LAGG_PORT_SLAVE		0
#define LAGG_PORT_MASTER	__BIT(0)
#define LAGG_PORT_STACK		__BIT(1)
#define LAGG_PORT_ACTIVE	__BIT(2)
#define LAGG_PORT_COLLECTING	__BIT(3)
#define LAGG_PORT_DISTRIBUTING	__BIT(4)
#define LAGG_PORT_STANDBY	__BIT(5)
#define LAGG_PORT_BITS			\
	"\020"				\
	"\001MASTER"			\
	"\002STACK"			\
	"\003ACTIVE"			\
	"\004COLLECTING"		\
	"\005DISTRIBUTING"		\
	"\006STANDBY"
#define LACP_PORTSTR_LEN	256

struct laggreq_lacpport {
	uint16_t	 partner_prio;
	uint8_t		 partner_mac[LACP_MAC_LEN];
	uint16_t	 partner_key;

	uint16_t	 actor_portprio;
	uint16_t	 actor_portno;
	uint8_t		 actor_state;
	uint16_t	 partner_portprio;
	uint16_t	 partner_portno;
	uint8_t		 partner_state;
};

struct laggreqport {
	char		 rp_portname[IFNAMSIZ];
	uint32_t	 rp_prio;
	uint32_t	 rp_flags;

	union {
		struct laggreq_lacpport	 port_lacp;
	} rp_port;
#define rp_lacpport	rp_port.port_lacp
};

enum lagg_ioctl {
	LAGGIOC_NOCMD,
	LAGGIOC_SETPROTO = 1,
	LAGGIOC_SETPROTOOPT,
	LAGGIOC_ADDPORT,
	LAGGIOC_DELPORT,
	LAGGIOC_SETPORTPRI,
};

struct lagg_req {
	uint32_t		 lrq_ioctl;
	lagg_proto		 lrq_proto;
	size_t			 lrq_nports;
	struct laggreqproto	 lrq_reqproto;
	struct laggreqport	 lrq_reqports[1];
};

#define	SIOCGLAGG		SIOCGIFGENERIC
#define	SIOCSLAGG		SIOCSIFGENERIC
#define SIOCGLAGGPORT		SIOCGIFGENERIC
#endif
