#include <rpc/rpc.h>
#include "/usr/include/rpcsvc/amq.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

void *
amqproc_null_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static char res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_NULL, xdr_void, argp, xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&res);
}


amq_mount_tree_p *
amqproc_mnttree_1(argp, clnt)
	amq_string *argp;
	CLIENT *clnt;
{
	static amq_mount_tree_p res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_MNTTREE, xdr_amq_string, argp, xdr_amq_mount_tree_p, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


void *
amqproc_umnt_1(argp, clnt)
	amq_string *argp;
	CLIENT *clnt;
{
	static char res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_UMNT, xdr_amq_string, argp, xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&res);
}


amq_mount_stats *
amqproc_stats_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static amq_mount_stats res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_STATS, xdr_void, argp, xdr_amq_mount_stats, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


amq_mount_tree_list *
amqproc_export_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static amq_mount_tree_list res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_EXPORT, xdr_void, argp, xdr_amq_mount_tree_list, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


int *
amqproc_setopt_1(argp, clnt)
	amq_setopt *argp;
	CLIENT *clnt;
{
	static int res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_SETOPT, xdr_amq_setopt, argp, xdr_int, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


amq_mount_info_list *
amqproc_getmntfs_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static amq_mount_info_list res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_GETMNTFS, xdr_void, argp, xdr_amq_mount_info_list, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


int *
amqproc_mount_1(argp, clnt)
	amq_string *argp;
	CLIENT *clnt;
{
	static int res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_MOUNT, xdr_amq_string, argp, xdr_int, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


amq_string *
amqproc_getvers_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static amq_string res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, AMQPROC_GETVERS, xdr_void, argp, xdr_amq_string, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

