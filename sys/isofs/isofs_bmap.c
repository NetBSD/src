/*
 * 	$Id: isofs_bmap.c,v 1.5 1993/10/28 17:38:43 ws Exp $
 */

#include "param.h"
#include "namei.h"
#include "buf.h"
#include "file.h"
#include "vnode.h"
#include "mount.h"

#include "iso.h"
#include "isofs_node.h"

iso_bmap(ip, lblkno, result)
struct iso_node *ip;
int lblkno;
daddr_t *result;
{
	*result = (ip->iso_start + lblkno)
		  * (ip->i_mnt->logical_block_size / DEV_BSIZE);
	return 0;
}
