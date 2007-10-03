/* $NetBSD: ctrl_if.h,v 1.3.46.1 2007/10/03 19:26:03 garbled Exp $ */
/******************************************************************************
 * ctrl_if.h
 * 
 * Management functions for special interface to the domain controller.
 * 
 * Copyright (c) 2004, K A Fraser
 */

#ifndef __ASM_XEN__CTRL_IF_H__
#define __ASM_XEN__CTRL_IF_H__

typedef control_msg_t ctrl_msg_t;

/*
 * Callback function type. Called for asynchronous processing of received
 * request messages, and responses to previously-transmitted request messages.
 * The parameters are (@msg, @id).
 *  @msg: Original request/response message (not a copy). The message can be
 *        modified in-place by the handler (e.g., a response callback can
 *        turn a request message into a response message in place). The message
 *        is no longer accessible after the callback handler returns -- if the
 *        message is required to persist for longer then it must be copied.
 *  @id:  (Response callbacks only) The 'id' that was specified when the
 *        original request message was queued for transmission.
 */
typedef void (*ctrl_msg_handler_t)(ctrl_msg_t *, unsigned long);

/*
 * Send @msg to the domain controller. Execute @hnd when a response is
 * received, passing the response message and the specified @id. This
 * operation will not block: it will return -EAGAIN if there is no space.
 * Notes:
 *  1. The @msg is copied if it is transmitted and so can be freed after this
 *     function returns.
 *  2. If @hnd is NULL then no callback is executed.
 */
int
ctrl_if_send_message_noblock(
    ctrl_msg_t *msg, 
    ctrl_msg_handler_t hnd,
    unsigned long id);

/*
 * Send @msg to the domain controller. Execute @hnd when a response is
 * received, passing the response message and the specified @id. This
 * operation will block until the message is sent, or a signal is received
 * for the calling process (unless @wait_state is TASK_UNINTERRUPTIBLE).
 * Notes:
 *  1. The @msg is copied if it is transmitted and so can be freed after this
 *     function returns.
 *  2. If @hnd is NULL then no callback is executed.
 */
int
ctrl_if_send_message_block(
    ctrl_msg_t *msg, 
    ctrl_msg_handler_t hnd, 
    unsigned long id, 
    long wait_state);

/*
 * Send @msg to the domain controller. Block until the response is received,
 * and then copy it into the provided buffer, @rmsg.
 */
int
ctrl_if_send_message_and_get_response(
    ctrl_msg_t *msg,
    ctrl_msg_t *rmsg,
    long wait_state);

#ifdef notyet
/*
 * Request a callback when there is /possibly/ space to immediately send a
 * message to the domain controller. This function returns 0 if there is
 * already space to trasnmit a message --- in this case the callback task /may/
 * still be executed. If this function returns 1 then the callback /will/ be
 * executed when space becomes available.
 */
int
ctrl_if_enqueue_space_callback(
    struct tq_struct *task);
#endif

/*
 * Send a response (@msg) to a message from the domain controller. This will 
 * never block.
 * Notes:
 *  1. The @msg is copied and so can be freed after this function returns.
 *  2. The @msg may be the original request message, modified in-place.
 */
void
ctrl_if_send_response(
    ctrl_msg_t *msg);

/*
 * Register a receiver for typed messages from the domain controller. The 
 * handler (@hnd) is called for every received message of specified @type.
 * Returns TRUE (non-zero) if the handler was successfully registered.
 * If CALLBACK_IN_BLOCKING CONTEXT is specified in @flags then callbacks will
 * occur in a context in which it is safe to yield (i.e., process context).
 */
#define CALLBACK_IN_BLOCKING_CONTEXT 1
int ctrl_if_register_receiver(
    uint8_t type, 
    ctrl_msg_handler_t hnd,
    unsigned int flags);

/*
 * Unregister a receiver for typed messages from the domain controller. The 
 * handler (@hnd) will not be executed after this function returns.
 */
void
ctrl_if_unregister_receiver(
    uint8_t type, ctrl_msg_handler_t hnd);

/* Suspend/resume notifications. */
void ctrl_if_suspend(void);
void ctrl_if_resume(void);

/* Start-of-day setup. */
void ctrl_if_early_init(void);
void ctrl_if_init(void);

/*
 * Returns TRUE if there are no outstanding message requests at the domain
 * controller. This can be used to ensure that messages have really flushed
 * through when it is not possible to use the response-callback interface.
 * WARNING: If other subsystems are using the control interface then this
 * function might never return TRUE!
 */
int ctrl_if_transmitter_empty(void);  /* !! DANGEROUS FUNCTION !! */

/*
 * Manually discard response messages from the domain controller. 
 * WARNING: This is usually done automatically -- this function should only
 * be called when normal interrupt mechanisms are disabled!
 */
void ctrl_if_discard_responses(void); /* !! DANGEROUS FUNCTION !! */

/*
 * message polling for use by xencons. Warning: this will reenable
 * interrupts at splhigh.
 */
void ctrl_if_console_poll(void);

#endif /* __ASM_XEN__CONTROL_IF_H__ */
