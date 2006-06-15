/*	$NetBSD: ctrl_if.c,v 1.2.2.10 2006/06/15 13:33:25 tron Exp $	*/

/******************************************************************************
 * ctrl_if.c
 * 
 * Management functions for special interface to the domain controller.
 * 
 * Copyright (c) 2004, K A Fraser
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ctrl_if.c,v 1.2.2.10 2006/06/15 13:33:25 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/malloc.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/ctrl_if.h>
#include <machine/evtchn.h>

#if 0
#define DPRINTK(_f, _a...) printk("(file=%s, line=%d) " _f, \
                           __FILE__ , __LINE__ , ## _a )
#else
#define DPRINTK(_f, _a...) ((void)0)
#endif

/*
 * Only used by initial domain which must create its own control-interface
 * event channel. This value is picked up by the user-space domain controller
 * via an ioctl.
 */
int initdom_ctrlif_domcontroller_port = -1;

/* static */ int ctrl_if_evtchn = -1;
static struct simplelock ctrl_if_lock;

static CONTROL_RING_IDX ctrl_if_tx_resp_cons;
static CONTROL_RING_IDX ctrl_if_rx_req_cons;

/* Incoming message requests. */
    /* Primary message type -> message handler. */
static ctrl_msg_handler_t ctrl_if_rxmsg_handler[256];
    /* Primary message type -> callback in process context? */
static unsigned long ctrl_if_rxmsg_blocking_context[256/sizeof(unsigned long)];
    /* Queue up messages to be handled in process context. */
static ctrl_msg_t ctrl_if_rxmsg_deferred[CONTROL_RING_SIZE];
static CONTROL_RING_IDX ctrl_if_rxmsg_deferred_prod;
static CONTROL_RING_IDX ctrl_if_rxmsg_deferred_cons;

/* Incoming message responses: message identifier -> message handler/id. */
static struct {
    ctrl_msg_handler_t fn;
    unsigned long      id;
} ctrl_if_txmsg_id_mapping[CONTROL_RING_SIZE];

/* For received messages that must be deferred to process context. */
static void __ctrl_if_rxmsg_deferred(void *unused);
static void ctrl_if_kthread_create(void *);

static int ctrl_if_tx_wait;
static void __ctrl_if_tx_tasklet(unsigned long data);

static void __ctrl_if_rx_tasklet(unsigned long data);

#define get_ctrl_if() ((control_if_t *)((uintptr_t)HYPERVISOR_shared_info + 2048))
#define TX_FULL(_c)   \
    (((_c)->tx_req_prod - ctrl_if_tx_resp_cons) == CONTROL_RING_SIZE)

static void ctrl_if_notify_controller(void)
{
    hypervisor_notify_via_evtchn(ctrl_if_evtchn);
}

static void ctrl_if_rxmsg_default_handler(ctrl_msg_t *msg, unsigned long id)
{
    msg->length = 0;
    ctrl_if_send_response(msg);
}

static void
__ctrl_if_tx_tasklet(unsigned long data)
{
    control_if_t *ctrl_if = get_ctrl_if();
    ctrl_msg_t   *msg;
    int           was_full = TX_FULL(ctrl_if);
    CONTROL_RING_IDX rp;

    rp = ctrl_if->tx_resp_prod;
    x86_lfence(); /* Ensure we see all requests up to 'rp'. */

    while ( ctrl_if_tx_resp_cons != rp )
    {
        msg = &ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if_tx_resp_cons)];

        DPRINTK("Rx-Rsp %u/%u :: %d/%d\n", 
                ctrl_if_tx_resp_cons,
                ctrl_if->tx_resp_prod,
                msg->type, msg->subtype);

        /* Execute the callback handler, if one was specified. */
        if ( msg->id != 0xFF )
        {
            (*ctrl_if_txmsg_id_mapping[msg->id].fn)(
                msg, ctrl_if_txmsg_id_mapping[msg->id].id);
            __insn_barrier(); /* Execute, /then/ free. */
            ctrl_if_txmsg_id_mapping[msg->id].fn = NULL;
        }

        /*
         * Step over the message in the ring /after/ finishing reading it. As 
         * soon as the index is updated then the message may get blown away.
         */
        __insn_barrier();
        ctrl_if_tx_resp_cons++;
    }

    if ( was_full && !TX_FULL(ctrl_if) )
    {
        wakeup(&ctrl_if_tx_wait);
    }
}

static void
__ctrl_if_rxmsg_deferred(void *unused)
{
	ctrl_msg_t *msg;
	CONTROL_RING_IDX dp;
	int s;


	while (1) {
		s = splsoftnet();
		dp = ctrl_if_rxmsg_deferred_prod;
		x86_lfence(); /* Ensure we see all requests up to 'dp'. */
		if (ctrl_if_rxmsg_deferred_cons == dp) {
			tsleep(&ctrl_if_rxmsg_deferred_cons, PRIBIO,
			    "rxdef", 0);
			splx(s);
			continue;
		}
		splx(s);

		while ( ctrl_if_rxmsg_deferred_cons != dp )
		{
			msg = &ctrl_if_rxmsg_deferred[
			    MASK_CONTROL_IDX(ctrl_if_rxmsg_deferred_cons)];
			(*ctrl_if_rxmsg_handler[msg->type])(msg, 0);
			ctrl_if_rxmsg_deferred_cons++;
		}
	}
}

static void
__ctrl_if_rx_tasklet(unsigned long data)
{
    control_if_t *ctrl_if = get_ctrl_if();
    ctrl_msg_t    msg, *pmsg;
    CONTROL_RING_IDX rp, dp;

    dp = ctrl_if_rxmsg_deferred_prod;
    rp = ctrl_if->rx_req_prod;
    x86_lfence(); /* Ensure we see all requests up to 'rp'. */

    while ( ctrl_if_rx_req_cons != rp )
    {
        pmsg = &ctrl_if->rx_ring[MASK_CONTROL_IDX(ctrl_if_rx_req_cons)];
        memcpy(&msg, pmsg, offsetof(ctrl_msg_t, msg));

        DPRINTK("Rx-Req %u/%u :: %d/%d\n", 
                ctrl_if_rx_req_cons-1,
                ctrl_if->rx_req_prod,
                msg.type, msg.subtype);

        if ( msg.length != 0 )
            memcpy(msg.msg, pmsg->msg, msg.length);
	/*
	 * increase ctrl_if_rx_req_cons now,
	 * as the handler may end up calling __ctrl_if_rx_tasklet() again
	 * though the console polling code.
	 */
	ctrl_if_rx_req_cons++;
        if ( xen_atomic_test_bit(
                      (unsigned long *)&ctrl_if_rxmsg_blocking_context,
		      msg.type) )
            memcpy(&ctrl_if_rxmsg_deferred[MASK_CONTROL_IDX(dp++)],
                   &msg, offsetof(ctrl_msg_t, msg) + msg.length);
        else
            (*ctrl_if_rxmsg_handler[msg.type])(&msg, 0);
	/* update rp, in case the console polling code was used */
    	rp = ctrl_if->rx_req_prod;
    	x86_lfence(); /* Ensure we see all requests up to 'rp'. */
    }

    if ( dp != ctrl_if_rxmsg_deferred_prod )
    {
        __insn_barrier();
        ctrl_if_rxmsg_deferred_prod = dp;
        wakeup(&ctrl_if_rxmsg_deferred_cons);
    }
}

static int ctrl_if_interrupt(void *arg)
{
	control_if_t *ctrl_if = get_ctrl_if();
	int ret = 0;

	if ( ctrl_if_tx_resp_cons != ctrl_if->tx_resp_prod ) {
		ret = 1;
		__ctrl_if_tx_tasklet(0);
	}

	if ( ctrl_if_rx_req_cons != ctrl_if->rx_req_prod ) {
		ret = 1;
		__ctrl_if_rx_tasklet(0);
	}

	return ret;
}

int
ctrl_if_send_message_noblock(
    ctrl_msg_t *msg, 
    ctrl_msg_handler_t hnd,
    unsigned long id)
{
    control_if_t *ctrl_if = get_ctrl_if();
    unsigned long flags;
    int           i;
    int s;

    save_and_cli(flags);
    simple_lock(&ctrl_if_lock);

    while ( TX_FULL(ctrl_if) )
    {
        simple_unlock(&ctrl_if_lock);
	restore_flags(flags);
	s = splhigh();
	if ( ctrl_if_tx_resp_cons != ctrl_if->tx_resp_prod ) {
		__ctrl_if_tx_tasklet(0);
		splx(s);
		save_and_cli(flags);
		simple_lock(&ctrl_if_lock);
	} else {
		splx(s);
        	return EAGAIN;
	}
    }

    msg->id = 0xFF;
    if ( hnd != NULL )
    {
        for ( i = 0; ctrl_if_txmsg_id_mapping[i].fn != NULL; i++ )
            continue;
        ctrl_if_txmsg_id_mapping[i].fn = hnd;
        ctrl_if_txmsg_id_mapping[i].id = id;
        msg->id = i;
    }

    DPRINTK("Tx-Req %u/%u :: %d/%d\n", 
            ctrl_if->tx_req_prod, 
            ctrl_if_tx_resp_cons,
            msg->type, msg->subtype);

    memcpy(&ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)], 
           msg, sizeof(*msg));
    x86_lfence(); /* Write the message before letting the controller peek at it. */
    ctrl_if->tx_req_prod++;

    simple_unlock(&ctrl_if_lock);
    restore_flags(flags);

    ctrl_if_notify_controller();

    return 0;
}

int
ctrl_if_send_message_block(
    ctrl_msg_t *msg, 
    ctrl_msg_handler_t hnd, 
    unsigned long id,
    long wait_state)
{
	int rc;

	while ((rc = ctrl_if_send_message_noblock(msg, hnd, id)) == EAGAIN) {
		/* XXXcl possible race -> add a lock and ltsleep */
#if 1
		HYPERVISOR_yield();
#else
		rc = tsleep((caddr_t) &ctrl_if_tx_wait, PUSER | PCATCH,
		    "ctrl_if", 0);
		if (rc)
			break;
#endif
	}

	return rc;
}

/* Allow a reponse-callback handler to find context of a blocked requester.  */
struct rsp_wait {
    ctrl_msg_t         *msg;  /* Buffer for the response message.            */
    struct task_struct *task; /* The task that is blocked on the response.   */
    int                 done; /* Indicate to 'task' that response is rcv'ed. */
};

static void __ctrl_if_get_response(ctrl_msg_t *msg, unsigned long id)
{
    struct rsp_wait    *wait = (struct rsp_wait *)id;

    memcpy(wait->msg, msg, sizeof(*msg));
    x86_lfence();
    wait->done = 1;

    wakeup(wait);
}

int
ctrl_if_send_message_and_get_response(
    ctrl_msg_t *msg, 
    ctrl_msg_t *rmsg,
    long wait_state)
{
    struct rsp_wait wait;
    int rc;

    wait.msg  = rmsg;
    wait.done = 0;

    if ( (rc = ctrl_if_send_message_block(msg, __ctrl_if_get_response,
                                          (unsigned long)&wait,
                                          wait_state)) != 0 )
        return rc;

    for ( ; ; )
    {
	    if ( wait.done )
		    break;
	    tsleep((caddr_t)&wait, PUSER | PCATCH, "ctrl_if", 0);
    }

    return 0;
}

#ifdef notyet
int
ctrl_if_enqueue_space_callback(
    struct tq_struct *task)
{
    control_if_t *ctrl_if = get_ctrl_if();

    /* Fast path. */
    if ( !TX_FULL(ctrl_if) )
        return 0;

    (void)queue_task(task, &ctrl_if_tx_tq);

    /*
     * We may race execution of the task queue, so return re-checked status. If
     * the task is not executed despite the ring being non-full then we will
     * certainly return 'not full'.
     */
    x86_lfence();
    return TX_FULL(ctrl_if);
}
#endif

void
ctrl_if_send_response(
    ctrl_msg_t *msg)
{
    control_if_t *ctrl_if = get_ctrl_if();
    unsigned long flags;
    ctrl_msg_t   *dmsg;

    /*
     * NB. The response may the original request message, modified in-place.
     * In this situation we may have src==dst, so no copying is required.
     */
    save_and_cli(flags);
    simple_lock(&ctrl_if_lock);

    DPRINTK("Tx-Rsp %u :: %d/%d\n", 
            ctrl_if->rx_resp_prod, 
            msg->type, msg->subtype);

    dmsg = &ctrl_if->rx_ring[MASK_CONTROL_IDX(ctrl_if->rx_resp_prod)];
    if ( dmsg != msg )
        memcpy(dmsg, msg, sizeof(*msg));

    x86_lfence(); /* Write the message before letting the controller peek at it. */
    ctrl_if->rx_resp_prod++;

    simple_unlock(&ctrl_if_lock);
    restore_flags(flags);

    ctrl_if_notify_controller();
}

int
ctrl_if_register_receiver(
    uint8_t type, 
    ctrl_msg_handler_t hnd, 
    unsigned int flags)
{
    unsigned long _flags;
    int inuse;

    save_and_cli(_flags);
    simple_lock(&ctrl_if_lock);

    inuse = (ctrl_if_rxmsg_handler[type] != ctrl_if_rxmsg_default_handler);

    if ( inuse )
    {
        printf("Receiver %p already established for control "
               "messages of type %d.\n", ctrl_if_rxmsg_handler[type], type);
    }
    else
    {
        ctrl_if_rxmsg_handler[type] = hnd;
        xen_atomic_clear_bit((unsigned long *)&ctrl_if_rxmsg_blocking_context, type);
        if ( flags == CALLBACK_IN_BLOCKING_CONTEXT )
        {
            xen_atomic_set_bit((unsigned long *)&ctrl_if_rxmsg_blocking_context, type);
#if 0
            if ( !safe_to_schedule_task )
                BUG();
#endif
        }
    }

    simple_unlock(&ctrl_if_lock);
    restore_flags(_flags);

    return !inuse;
}

void 
ctrl_if_unregister_receiver(
    uint8_t type,
    ctrl_msg_handler_t hnd)
{
    unsigned long flags;

    save_and_cli(flags);
    simple_lock(&ctrl_if_lock);

    if ( ctrl_if_rxmsg_handler[type] != hnd )
        printf("Receiver %p is not registered for control "
               "messages of type %d.\n", hnd, type);
    else
        ctrl_if_rxmsg_handler[type] = ctrl_if_rxmsg_default_handler;

    simple_unlock(&ctrl_if_lock);
    restore_flags(flags);

    /* Ensure that @hnd will not be executed after this function returns. */
    wakeup(&ctrl_if_rxmsg_deferred_cons);
}

#ifdef notyet
void ctrl_if_suspend(void)
{
    free_irq(ctrl_if_irq, NULL);
    unbind_evtchn_from_irq(ctrl_if_evtchn);
}
#endif

void ctrl_if_resume(void)
{
    control_if_t *ctrl_if = get_ctrl_if();

    if ( xen_start_info.flags & SIF_INITDOMAIN )
    {
        /*
         * The initial domain must create its own domain-controller link.
         * The controller is probably not running at this point, but will
         * pick up its end of the event channel from 
         */
        evtchn_op_t op;
        op.cmd = EVTCHNOP_bind_interdomain;
        op.u.bind_interdomain.dom1 = DOMID_SELF;
        op.u.bind_interdomain.dom2 = DOMID_SELF;
	op.u.bind_interdomain.port1 = 0;
	op.u.bind_interdomain.port2 = 0;
        if ( HYPERVISOR_event_channel_op(&op) != 0 )
		panic("EVTCHNOP_bind_interdomain");
        xen_start_info.domain_controller_evtchn = op.u.bind_interdomain.port1;
        initdom_ctrlif_domcontroller_port   = op.u.bind_interdomain.port2;
    }

    /* Sync up with shared indexes. */
    ctrl_if_tx_resp_cons = ctrl_if->tx_resp_prod;
    ctrl_if_rx_req_cons  = ctrl_if->rx_resp_prod;

    ctrl_if_evtchn = xen_start_info.domain_controller_evtchn;
    aprint_verbose("Domain controller: using event channel %d\n",
	ctrl_if_evtchn);

    event_set_handler(ctrl_if_evtchn, &ctrl_if_interrupt, NULL, IPL_SCHED,
	"ctrlev");
    hypervisor_enable_event(ctrl_if_evtchn);
}

void ctrl_if_early_init(void)
{

	simple_lock_init(&ctrl_if_lock);

	ctrl_if_evtchn = xen_start_info.domain_controller_evtchn;
}

void ctrl_if_init(void)
{
	int i;

	for ( i = 0; i < 256; i++ )
		ctrl_if_rxmsg_handler[i] = ctrl_if_rxmsg_default_handler;

	if (ctrl_if_evtchn == -1)
		ctrl_if_early_init();

	kthread_create(ctrl_if_kthread_create, NULL);

	ctrl_if_resume();
}

static void
ctrl_if_kthread_create(void *arg)
{
	int error;
	static struct proc *ctrl_if_proc;
	if ((error = kthread_create1(__ctrl_if_rxmsg_deferred, NULL,
	    &ctrl_if_proc, "ctrlif")) != 0) {
		aprint_error("ctrlif: unable to create kernel thread: "
		    "error %d\n", error);
	}
}


/*
 * !! The following are DANGEROUS FUNCTIONS !!
 * Use with care [for example, see xencons_force_flush()].
 */

int ctrl_if_transmitter_empty(void)
{
    return (get_ctrl_if()->tx_req_prod == ctrl_if_tx_resp_cons);
}

void ctrl_if_discard_responses(void)
{
    ctrl_if_tx_resp_cons = get_ctrl_if()->tx_resp_prod;
}

/*
 * polling functions, for xencons's benefit
 * We need to enable events before calling HYPERVISOR_block().
 * Do this at a high enouth IPL so that no other interrupt will interfere
 * with our polling.
 */

void
ctrl_if_console_poll(void)
{
	int s = splhigh();

	/*
	 * Unmask the ctrl event, and block waiting for an interrupt.
	 * Note that the event we get may not be for the console.
	 * In this case, the console code will call us again.
	 */

	while (ctrl_if_interrupt(NULL) == 0) {
		/* enable interrupts. */
		hypervisor_clear_event(ctrl_if_evtchn);
		hypervisor_unmask_event(ctrl_if_evtchn);
		HYPERVISOR_block();
	}
	splx(s);
}
