/*
 * 	$Id: isofs_bmap.c,v 1.4 1993/09/07 15:40:53 ws Exp $
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
	*result = (ip->i_number + lblkno)
		  * (ip->i_mnt->im_bsize / DEV_BSIZE);
	return (0);
}
