#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/mbuf.h>

#include <net/bpf.h>

static void
bpf_stub_attach(struct ifnet *ipf, u_int dlt, u_int hlen, struct bpf_if **drvp)
{

	*drvp = NULL;
}

static void
bpf_stub_null(void)
{

}

static void
bpf_stub_warn(void)
{

#ifdef DEBUG
	panic("bpf method called without attached bpf_if");
#endif
#ifdef DIAGNOSTIC
	printf("bpf method called without attached bpf_if\n");
#endif
}

struct bpf_ops bpf_ops_stub = {
	.bpf_attach =		bpf_stub_attach,
	.bpf_detach =		(void *)bpf_stub_null,
	.bpf_change_type =	(void *)bpf_stub_null,

	.bpf_tap = 		(void *)bpf_stub_warn,
	.bpf_mtap = 		(void *)bpf_stub_warn,
	.bpf_mtap2 = 		(void *)bpf_stub_warn,
	.bpf_mtap_af = 		(void *)bpf_stub_warn,
	.bpf_mtap_et = 		(void *)bpf_stub_warn,
	.bpf_mtap_sl_in = 	(void *)bpf_stub_warn,
	.bpf_mtap_sl_out =	(void *)bpf_stub_warn,
};

struct bpf_ops *bpf_ops;

void bpf_setops_stub(void);
void
bpf_setops_stub()
{

	bpf_ops = &bpf_ops_stub;
}
__weak_alias(bpf_setops,bpf_setops_stub);
