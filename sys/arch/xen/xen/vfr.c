/* $NetBSD: vfr.c,v 1.1 2004/05/07 15:51:04 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfr.c,v 1.1 2004/05/07 15:51:04 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>

#include <lib/libkern/libkern.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>

#include <machine/kernfs_machdep.h>

#define	VFR_MODE	(S_IRUSR)


static char *
getword(char *str[], size_t *len)
{
	char *word;

	while (*len > 0 && isspace(*str[0])) {
		(*str)++;
		(*len)--;
	}
	if (*len == 0)
		return NULL;
	word = *str;
	while (*len > 0 && !isspace(*str[0])) {
		(*str)++;
		(*len)--;
	}
	if (*len > 0) {
		*str[0] = 0;
		(*str)++;
		(*len)--;
	}
	return word;
}

static int
getkeyval(char *str[], size_t *len, char *key[], char *val[])
{
	char *s;

	*key = getword(str, len);
	if (*key == NULL)
		return 1;
	s = strchr(*key, '=');
	if (s == *key)
		return 1;
	if (s == NULL) {
		s = getword(str, len);
		if (s == NULL || *s != '=')
			return 1;
	}
	while (*s == '=')
		*s++ = 0;
	if (*s) {
		*val = s;
		return 0;
	}
	*val = getword(str, len);
	if (*val == NULL)
		return 1;
	return 0;
}

static long
strtol(const char *nptr, char **endptr,	int base)
{

	return (long)strtoul(nptr, endptr, base);
}

static u_int32_t
host_addr (const char *host)
{
	return ntohl(inet_addr(host));
}

static int
vfr_xwrite(const struct kernfs_node *kfs, char *buf, size_t len)
{
	network_op_t op;
	char *action, *cmd, *key, *val;

	memset(&op, 0, sizeof(network_op_t));

	cmd = getword(&buf, &len);
	if (cmd == NULL)
		return 0;

	if (strcmp(cmd, "ADD") == 0)
		op.cmd = NETWORK_OP_ADDRULE;
	else if (strcmp(cmd, "DELETE") == 0)
		op.cmd = NETWORK_OP_DELETERULE;
	else if (strcmp(cmd, "PRINT") == 0) {
		op.cmd = NETWORK_OP_GETRULELIST;
		goto make_op;
	}

	if (len == 0)
		return 0;

	action = getword(&buf, &len);
	if (action == NULL)
		return 0;

	if (strcmp(action, "ACCEPT") == 0)
		op.u.net_rule.action = NETWORK_ACTION_ACCEPT;
	else if (strcmp(action, "COUNT") == 0)
		op.u.net_rule.action = NETWORK_ACTION_COUNT;

	if (len == 0)
		return 0;

	while (len > 0) {
		if (getkeyval(&buf, &len, &key, &val)) {
			if (key)
				return 0;
			else
				break;
		}

		if (strcmp(key, "src") == 0)
			op.u.net_rule.src_vif = VIF_ANY_INTERFACE;
		else if (strcmp(key, "dst") == 0)
			op.u.net_rule.dst_vif = VIF_PHYSICAL_INTERFACE;
		else if (strcmp(key, "srcaddr") == 0) 
			op.u.net_rule.src_addr = host_addr(val);
		else if (strcmp(key, "dstaddr") == 0)
			op.u.net_rule.dst_addr = host_addr(val);
		else if (strcmp(key, "srcaddrmask") == 0) 
			op.u.net_rule.src_addr_mask = host_addr(val);
		else if (strcmp(key, "dstaddrmask") == 0)
			op.u.net_rule.dst_addr_mask = host_addr(val);
		else if (strcmp(key, "srcport") == 0)
			op.u.net_rule.src_port = strtoul(val, NULL, 10);
		else if (strcmp(key, "dstport") == 0)
			op.u.net_rule.dst_port = strtoul(val, NULL, 10);
		else if (strcmp(key, "srcportmask") == 0)
			op.u.net_rule.src_port_mask = strtoul(val, NULL, 10);
		else if (strcmp(key, "dstportmask") == 0)
			op.u.net_rule.dst_port_mask = strtoul(val, NULL, 10);
		else if (strcmp(key, "srcdom") == 0)
			op.u.net_rule.src_vif |=
				strtol(val, NULL, 10) << VIF_DOMAIN_SHIFT;
		else if (strcmp(key, "srcidx") == 0)
			op.u.net_rule.src_vif |= strtol(val, NULL, 10);
		else if (strcmp(key, "dstdom") == 0)
			op.u.net_rule.dst_vif |=
				strtol(val, NULL, 10) << VIF_DOMAIN_SHIFT;
		else if (strcmp(key, "dstidx") == 0)
			op.u.net_rule.dst_vif |= strtol(val, NULL, 10);
		else if (strcmp(key, "proto") == 0) {
			if (strcmp(val, "any") == 0) 
				op.u.net_rule.proto = NETWORK_PROTO_ANY; 
			if (strcmp(val, "ip") == 0)
				op.u.net_rule.proto = NETWORK_PROTO_IP;
			if (strcmp(val, "tcp") == 0) 
				op.u.net_rule.proto = NETWORK_PROTO_TCP;
			if (strcmp(val, "udp") == 0)
				op.u.net_rule.proto = NETWORK_PROTO_UDP;
			if (strcmp(val, "arp") == 0)
				op.u.net_rule.proto = NETWORK_PROTO_ARP;
		}
	}

 make_op:
	HYPERVISOR_network_op(&op);
	return 0;
}

#define	KSTRING 256
static int
vfr_write(void *v)
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	struct uio *uio = ap->a_uio;
	int error, xlen;
	char strbuf[KSTRING];


	xlen = min(uio->uio_resid, KSTRING-1);
	if ((error = uiomove(strbuf, xlen, uio)) != 0)
		return (error);

	if (uio->uio_resid != 0)
		return (EIO);

	strbuf[xlen] = '\0';
	xlen = strlen(strbuf);
	return (vfr_xwrite(kfs, strbuf, xlen));
}

static const struct kernfs_fileop vfr_fileops[] = {
	{ .kf_fileop = KERNFS_FILEOP_WRITE, .kf_vop = vfr_write },
};

void
xenvfr_init()
{
	kernfs_entry_t *dkt;
	kfstype kfst;

	if ((xen_start_info.flags & SIF_PRIVILEGED) == 0)
		return;

	kfst = KERNFS_ALLOCTYPE(vfr_fileops);

	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, "vfr", NULL, kfst, VREG, VFR_MODE);
	kernfs_addentry(kernxen_pkt, dkt);
}
