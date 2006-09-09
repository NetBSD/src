/*	$NetBSD: isakmp_unity.h,v 1.4 2006/09/09 16:22:09 manu Exp $	*/

/*	$KAME$ */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* ISAKMP notifies specific to the Unity vendor Id */
/* Sent during xauth if the user types his password too slowly */
#define ISAKMP_NTYPE_UNITY_HEARTBEAT	40500

/* ISAKMP mode config attributes specific to the Unity vendor Id */
#define UNITY_BANNER		28672
#define UNITY_SAVE_PASSWD	28673
#define UNITY_DEF_DOMAIN	28674
#define UNITY_SPLITDNS_NAME	28675
#define UNITY_SPLIT_INCLUDE	28676
#define UNITY_NATT_PORT		28677
#define UNITY_LOCAL_LAN		28678
#define UNITY_PFS		28679
#define UNITY_FW_TYPE		28680
#define UNITY_BACKUP_SERVERS	28681
#define UNITY_DDNS_HOSTNAME	28682

/*
 * Unity adress/mask lists
 * XXX : the padding is probably there for something !
 */
        
struct unity_network {
	struct in_addr addr4;
	struct in_addr mask4;
	char padding[6];
} __attribute__((__packed__));
        
struct unity_netentry {
	struct unity_network    network;       
	struct unity_netentry   *next;
};

int	splitnet_list_add(struct unity_netentry **, struct unity_network *, int *);
void	splitnet_list_free(struct unity_netentry *, int *);
char *	splitnet_list_2str(struct unity_netentry *);

vchar_t *isakmp_unity_req(struct ph1handle *, struct isakmp_data *);
void isakmp_unity_reply(struct ph1handle *, struct isakmp_data *);
