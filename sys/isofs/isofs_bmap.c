/*
 * 	$Id: isofs_bmap.c,v 1.4.2.3 1993/11/26 22:56:26 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <isofs/iso.h>
#include <isofs/isofs_node.h>

iso_bmap(ip, lblkno, result)
	struct iso_node *ip;
	int lblkno;
	daddr_t *result;
{

	*result = (ip->iso_start + lblkno)
		  * (ip->i_mnt->logical_block_size / DEV_BSIZE);
	return 0;
}
