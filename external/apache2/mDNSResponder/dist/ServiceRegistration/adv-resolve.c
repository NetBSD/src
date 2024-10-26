/* adv-resolve.c
 *
 * Copyright (c) 2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file contains the implementation of advertising proxy resolve
 * API, which allows applications that use DNSServiceResolve to signal to
 * the advertising proxy (as opposed to mDNSResponder) which services the
 * application is trying to resolve.
 */

#include <netdb.h>
#include <dns_sd.h>
#include <os/log.h>

#include "srp.h"
#include "advertising_proxy_services.h"

#undef ERROR
#define ERROR(fmt, ...) os_log_error(adv_log, "%{public}s: " # fmt, __FUNCTION__, ##__VA_ARGS__)
#undef INFO
#define INFO(fmt, ...) os_log_info(adv_log, "%{public}s: " # fmt, __FUNCTION__, ##__VA_ARGS__)
#undef FAULT
#define FAULT(fmt, ...) os_log_info(adv_log, "%{public}s: " # fmt, __FUNCTION__, ##__VA_ARGS__)

#define DECLARE_OBJECT_TYPE(x) int x##_created, x##_finalized, old_##x##_created, old_##x##_finalized
DECLARE_OBJECT_TYPE(adv_instance_state);
DECLARE_OBJECT_TYPE(adv_service_state);
DECLARE_OBJECT_TYPE(adv_resolver_state);
DECLARE_OBJECT_TYPE(advertising_proxy_subscription);

// Discovery domains seen
typedef struct discovery_domain discovery_domain_t;
typedef struct adv_instance_state adv_instance_state_t;
typedef struct adv_service_state adv_service_state_t;
typedef struct adv_resolver_state adv_resolver_state_t;

os_log_t advertising_proxy_global_os_log;
#define adv_log advertising_proxy_global_os_log

struct advertising_proxy_subscription {
    int ref_count;
    advertising_proxy_resolve_reply instance_callback;
    advertising_proxy_browse_reply service_callback;
    adv_instance_state_t *local_instance, *push_instance;
    adv_service_state_t *service;
    void *context;
};

static void
advertising_proxy_subscription_finalize(advertising_proxy_subscription_t *subscription)
{
    if (subscription->local_instance != NULL) {
        FAULT("subscription->local_instance is not NULL (%p)", subscription->local_instance);
    }
    if (subscription->push_instance != NULL) {
        FAULT("subscription->push_instance is not NULL (%p)", subscription->push_instance);
    }
    if (subscription->service != NULL) {
        FAULT("subscription->service is not NULL (%p)", subscription->service);
    }
}

// Retained state relating to queries for services of a particular type.
struct adv_service_state {
    int ref_count;
    adv_service_state_t *next;
    char *service_type;
    DNSServiceRef null_browse_ref; // default domains
    DNSServiceRef push_browse_ref;  // default.service.arpa
    adv_instance_state_t *instances;
    size_t max_subscribers;
    advertising_proxy_subscription_t **subscribers;
};

static void
adv_service_state_finalize(adv_service_state_t *state)
{
    free(state->service_type);
    if (state->null_browse_ref != NULL) {
        FAULT("state->null_browse_ref is non-null: %p", state->null_browse_ref);
        DNSServiceRefDeallocate(state->null_browse_ref);
    }
    if (state->push_browse_ref != NULL) {
        FAULT("state->null_browse_ref is non-null: %p", state->push_browse_ref);
        DNSServiceRefDeallocate(state->push_browse_ref);
    }
    if (state->instances != NULL) {
        FAULT("state->instances not NULL (%p)", state->instances);
    }
    if (state->next != NULL) {
        FAULT("state->next not NULL (%p)", state->next);
    }
    for (size_t i = 0; i < state->max_subscribers; i++) {
        if (state->subscribers[i] != NULL) {
            RELEASE_HERE(state->subscribers[i], advertising_proxy_subscription);
        }
    }
    free(state->subscribers);
}

static void
adv_service_state_cancel(adv_service_state_t *state)
{
    if (state->push_browse_ref != NULL) {
        DNSServiceRefDeallocate(state->push_browse_ref);
        state->push_browse_ref = NULL;
        RELEASE_HERE(state, adv_service_state); // No more callbacks possible.
    }
    if (state->null_browse_ref != NULL) {
        DNSServiceRefDeallocate(state->null_browse_ref);
        state->null_browse_ref = NULL;
        RELEASE_HERE(state, adv_service_state); // No more callbacks possible
    }
}

// State relating to an instance of a particular service.
struct adv_instance_state {
    int ref_count;
    adv_instance_state_t *next, *nsn; // nsn == next same name
    char *name;
    char *service_type;
    char *domain;
    DNSServiceRef resolve_ref;
#define INITIAL_MAX_SUBSCRIBERS 10 // Should usually be all we need
    size_t max_subscribers;
    advertising_proxy_subscription_t **subscribers;
};

static void
adv_instance_state_finalize(adv_instance_state_t *state)
{
    free(state->name);
    free(state->service_type);
    free(state->domain);
    if (state->resolve_ref != NULL) {
        FAULT("state->resolve_ref is non-null: %p", state->resolve_ref);
        DNSServiceRefDeallocate(state->resolve_ref); // shouldn't be possible
    }
    for (size_t i = 0; i < state->max_subscribers; i++) {
        if (state->subscribers[i] != NULL) {
            RELEASE_HERE(state->subscribers[i], advertising_proxy_subscription);
        }
    }
    free(state->subscribers);
    if (state->nsn != NULL) {
        FAULT("state->nsn is not NULL (%p)", state->nsn);
    }
    if (state->next != NULL) {
        FAULT("state->next is not NULL (%p)", state->next);
    }
}

static void
adv_instance_state_cancel(adv_instance_state_t *state)
{
    if (state->resolve_ref != NULL) {
        DNSServiceRefDeallocate(state->resolve_ref);
        state->resolve_ref = NULL;
        RELEASE_HERE(state, adv_instance_state); // No longer possible to get a callback
    }
}

// Call on startup to initialize the advertising proxy resolver function
advertising_proxy_error_type
advertising_proxy_resolver_init(os_log_t log_thingy)
{
    if (log_thingy != NULL) {
        adv_log = log_thingy;
    } else {
        adv_log = OS_LOG_DEFAULT;
    }

    // Any additional per-run setup...
    return kDNSSDAdvertisingProxyStatus_NoError;
}

static adv_service_state_t *
adv_service_state_create(const char *regtype)
{
    adv_service_state_t *state = calloc(1, sizeof(*state));
    if (state == NULL) {
        ERROR("no memory for service state %{public}s", regtype);
    }
    RETAIN_HERE(state, adv_service_state);
    state->service_type = strdup(regtype);
    if (state->service_type == NULL) {
        RELEASE_HERE(state, adv_service_state);
        state = NULL;
        ERROR("no memory for service_type %{public}s", regtype);
    }
    return state;
}

static adv_instance_state_t *
adv_instance_state_create(const char *name, const char *regtype, const char *domain)
{
    adv_instance_state_t *ret = NULL, *state = calloc(1, sizeof(*state));
    if (state == NULL) {
        ERROR("no memory for state %{private}s . %{public}s", name, regtype);
    } else {
        RETAIN_HERE(state, adv_instance_state);
        state->name = strdup(name);
        if (state->name == NULL) {
            ERROR("no memory for name %{private}s . %{public}s", name, regtype);
            goto out;
        }
        state->service_type = strdup(regtype);
        if (state->service_type == NULL) {
            ERROR("no memory for service type %{private}s . %{public}s", name, regtype);
            goto out;
        }
        if (domain != NULL) {
            state->domain = strdup(domain);
            if (state->domain == NULL) {
                ERROR("no memory for domain %{private}s . %{public}s", name, domain);
                goto out;
            }
        }
        ret = state;
        state = NULL;
    }
out:
    if (state != NULL) {
        RELEASE_HERE(state, adv_instance_state);
    }
    return ret;
}

static advertising_proxy_subscription_t *
advertising_proxy_subscription_create(void)
{
    advertising_proxy_subscription_t *subscription = calloc(1, sizeof(*subscription));
    if (subscription == NULL) {
        goto out;
    }
    RETAIN_HERE(subscription, advertising_proxy_subscription);
out:
    return subscription;
}


static void
adv_instance_unsubscribe(adv_instance_state_t *instance, advertising_proxy_subscription_t *subscription)
{
    if (instance->resolve_ref != NULL) {
        DNSServiceRefDeallocate(instance->resolve_ref);
        instance->resolve_ref = NULL;
    }
    for (size_t i = 0; i < instance->max_subscribers; i++) {
        if (instance->subscribers[i] == subscription) {
            RELEASE_HERE(instance->subscribers[i], advertising_proxy_subscription);
            instance->subscribers[i] = NULL;
            break;
        }
    }
}

static void
adv_service_unsubscribe(adv_service_state_t *service, advertising_proxy_subscription_t *subscription)
{
    if (service->null_browse_ref != NULL) {
        DNSServiceRefDeallocate(service->null_browse_ref);
        service->null_browse_ref = NULL;
    }
    if (service->push_browse_ref != NULL) {
        DNSServiceRefDeallocate(service->push_browse_ref);
        service->push_browse_ref = NULL;
    }
    for (size_t i = 0; i < service->max_subscribers; i++) {
        if (service->subscribers[i] == subscription) {
            RELEASE_HERE(service->subscribers[i], advertising_proxy_subscription);
            service->subscribers[i] = NULL;
            break;
        }
    }
}

advertising_proxy_error_type
advertising_proxy_subscription_retain_(advertising_proxy_subscription_t *subscription, const char *file, int line)
{
    RETAIN(subscription, advertising_proxy_subscription);
    return kDNSSDAdvertisingProxyStatus_NoError;
}

advertising_proxy_error_type
advertising_proxy_subscription_release_(advertising_proxy_subscription_t *subscription, const char *file, int line)
{
    RELEASE(subscription, advertising_proxy_subscription);
    return kDNSSDAdvertisingProxyStatus_NoError;
}

advertising_proxy_error_type
advertising_proxy_subscription_cancel(advertising_proxy_subscription_t *subscription)
{
    if (subscription->local_instance) {
        adv_instance_unsubscribe(subscription->local_instance, subscription);
        RELEASE_HERE(subscription->local_instance, adv_instance_state);
        subscription->local_instance = NULL;
    }
    if (subscription->push_instance) {
        adv_instance_unsubscribe(subscription->push_instance, subscription);
        RELEASE_HERE(subscription->push_instance, adv_instance_state);
        subscription->push_instance = NULL;
    }
    if (subscription->service) {
        adv_service_unsubscribe(subscription->service, subscription);
        RELEASE_HERE(subscription->service, adv_service_state);
        subscription->service = NULL;
    }
    return kDNSSDAdvertisingProxyStatus_NoError;
}

advertising_proxy_error_type
advertising_proxy_registrar_create(advertising_proxy_subscription_t **subscription_ret, run_context_t clientq,
                                   advertising_proxy_registrar_reply callback, void *context)
{
    run_context_t queue = clientq;
    advertising_proxy_error_type result = kDNSSDAdvertisingProxyStatus_NoError;
    advertising_proxy_subscription_t *subscription = NULL;
    // Sanity check arguments.
    if (subscription_ret == NULL || callback == NULL) {
        result = kDNSSDAdvertisingProxyStatus_Invalid;
        goto out;
    }
    if (queue == NULL) {
        queue = dispatch_get_main_queue();
    }

    subscription = advertising_proxy_subscription_create();

    if (subscription == NULL) {
        result = kDNSSDAdvertisingProxyStatus_NoMemory;
        goto out;
    }
    advertising_proxy_subscription_t *tmp = subscription;
    RETAIN_HERE(subscription, advertising_proxy_subscription); // For callback
    dispatch_async(clientq, ^{
        callback(tmp, kDNSSDAdvertisingProxyStatus_NoError, context);
        RELEASE_HERE(subscription, advertising_proxy_subscription); // For callback
    });
    *subscription_ret = subscription;
    subscription = NULL;

out:
    if (subscription != NULL) {
        RELEASE_HERE(subscription, advertising_proxy_subscription);
    }
    return result;
}

static void
adv_subscriber_add(size_t *num_subscribers, advertising_proxy_subscription_t ***p_subscribers,
                   advertising_proxy_subscription_t *subscriber)
{
    size_t max_subscribers = *num_subscribers;
    advertising_proxy_subscription_t **subscribers = *p_subscribers;

    for (size_t i = 0; i < max_subscribers; i++) {
        if (subscribers[i] == NULL) {
            subscribers[i] = subscriber;
            RETAIN_HERE(subscribers[i], advertising_proxy_subscription);
            return;
        }
    }
    size_t new_max = subscribers == NULL ? INITIAL_MAX_SUBSCRIBERS : max_subscribers * 2;
    advertising_proxy_subscription_t **new_subscribers = calloc(new_max, sizeof(*subscribers));
    if (new_subscribers == NULL) {
        ERROR("no memory for %zu subscribers", new_max);
        return;
    }
    if (*p_subscribers != NULL) {
        memcpy(new_subscribers, subscribers, max_subscribers * sizeof(*subscribers));
        free(*p_subscribers);
    }
    *p_subscribers = new_subscribers;
    new_subscribers[max_subscribers] = subscriber;
    RETAIN_HERE(new_subscribers[max_subscribers], advertising_proxy_subscription);
    *num_subscribers = new_max;
}

static void
adv_service_state_subscriber_add(adv_service_state_t *service, advertising_proxy_subscription_t *subscriber)
{
    adv_subscriber_add(&service->max_subscribers, &service->subscribers, subscriber);
}

static void
advertising_proxy_browse_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t interface_index,
                                  DNSServiceErrorType error_code,
                                  const char *instance_name, const char *service_type, const char *UNUSED domain, void *context)
{
    adv_service_state_t *service = context;
    advertising_proxy_error_type error = kDNSSDAdvertisingProxyStatus_NoError;

    RETAIN_HERE(service, adv_service_state);
    if (error_code != kDNSServiceErr_NoError) {
        error = kDNSSDAdvertisingProxyStatus_UnknownErr;
    }
    for (size_t i = 0; i < service->max_subscribers; i++) {
        advertising_proxy_subscription_t *subscription = service->subscribers[i];
        if (subscription != NULL) {
            subscription->service_callback(subscription, error, interface_index,
                                           (flags & kDNSServiceFlagsAdd) ? true : false,
                                           instance_name, service_type, context);
        }
    }
    if (error != kDNSSDAdvertisingProxyStatus_NoError) {
        adv_service_state_cancel(service);
    }
    RELEASE_HERE(service, adv_service_state);
}

advertising_proxy_error_type
advertising_proxy_browse_create(advertising_proxy_subscription_t **subscription_ret, run_context_t clientq,
                                const char *regtype, advertising_proxy_browse_reply callback, void *context)
{
    advertising_proxy_error_type status = kDNSSDAdvertisingProxyStatus_NoError;
    advertising_proxy_subscription_t *subscription = NULL;
    adv_service_state_t *service = NULL;

    // Sanity check arguments.
    if (subscription_ret == NULL || regtype == NULL || callback == NULL) {
        status = kDNSSDAdvertisingProxyStatus_Invalid;
        goto out;
    }

    run_context_t queue = clientq;
    if (queue == NULL) {
        queue = dispatch_get_main_queue();
    }

    subscription = advertising_proxy_subscription_create();
    if (subscription == NULL) {
        status = kDNSSDAdvertisingProxyStatus_NoMemory;
        goto out;
    }
    service = adv_service_state_create(regtype);
    if (service == NULL) {
        status = kDNSSDAdvertisingProxyStatus_NoMemory;
        goto out;
    }
    adv_service_state_subscriber_add(service, subscription);
    subscription->service = service;
    RETAIN_HERE(subscription->service, adv_service_state);

    subscription->service_callback = callback;
    subscription->context = context;

    DNSServiceErrorType error = DNSServiceBrowse(&service->null_browse_ref, 0, kDNSServiceInterfaceIndexAny,
                                                 regtype, NULL, advertising_proxy_browse_callback, service);
    if (error == kDNSServiceErr_NoError) {
        error = DNSServiceSetDispatchQueue(service->null_browse_ref, queue);
    }
    if (error != kDNSServiceErr_NoError) {
        ERROR("browse for service %{public}s in the default domains failed with %d", regtype, error);
        status = kDNSSDAdvertisingProxyStatus_UnknownErr;
        goto out;
    }
    RETAIN_HERE(service, adv_service_state); // For null_browse callback

    error = DNSServiceBrowse(&service->push_browse_ref, 0, kDNSServiceInterfaceIndexAny,
                             regtype, "default.service.arpa", advertising_proxy_browse_callback, service);
    if (error == kDNSServiceErr_NoError) {
        error = DNSServiceSetDispatchQueue(service->push_browse_ref, queue);
    }
    if (error != kDNSServiceErr_NoError) {
        ERROR("browse on service %{public}s in the push domain failed with %d", regtype, error);
        status = kDNSSDAdvertisingProxyStatus_UnknownErr;
        goto out;
    }
    RETAIN_HERE(service, adv_service_state); // For push_browse callback
    *subscription_ret = subscription;
    subscription = NULL;
    service = NULL;

out:
    if (subscription != NULL) {
        advertising_proxy_subscription_cancel(subscription);
        RELEASE_HERE(subscription, advertising_proxy_subscription);
    }
    if (service != NULL) {
        adv_service_state_cancel(service);
        RELEASE_HERE(service, adv_service_state);
    }
    return status;
}

static void
adv_instance_state_subscriber_add(adv_instance_state_t *instance, advertising_proxy_subscription_t *subscriber)
{
    adv_subscriber_add(&instance->max_subscribers, &instance->subscribers, subscriber);
}

static void
advertising_proxy_resolve_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t interface_index,
                                   DNSServiceErrorType error_code, const char *fullname, const char *hosttarget,
                                   uint16_t port, uint16_t txt_length, const unsigned char *txt_record, void *context)
{
    adv_instance_state_t *instance = context;
    advertising_proxy_error_type error = kDNSSDAdvertisingProxyStatus_NoError;

    RELEASE_HERE(instance, adv_instance_state);
    if (error_code != kDNSServiceErr_NoError) {
        error = kDNSSDAdvertisingProxyStatus_UnknownErr;
    }
    for (size_t i = 0; i < instance->max_subscribers; i++) {
        advertising_proxy_subscription_t *subscription = instance->subscribers[i];
        if (subscription != NULL) {
            subscription->instance_callback(subscription, error, interface_index,
                                            (flags & kDNSServiceFlagsAdd) ? true : true, fullname, hosttarget, port,
                                            txt_length, txt_record, context);
        }
    }
    if (error != kDNSSDAdvertisingProxyStatus_NoError) {
        adv_instance_state_cancel(instance);
    }
    RELEASE_HERE(instance, adv_instance_state);
}

advertising_proxy_error_type
advertising_proxy_resolve_create(advertising_proxy_subscription_t **subscription_ret, run_context_t clientq,
                                 const char *name, const char *regtype, const char *domain,
                                 advertising_proxy_resolve_reply callback, void *context)
{
    advertising_proxy_error_type status = kDNSSDAdvertisingProxyStatus_NoError;
    advertising_proxy_subscription_t *subscription = NULL;
    adv_instance_state_t *local_instance = NULL;
    adv_instance_state_t *push_instance = NULL;

    // Sanity check arguments.
    if (subscription_ret == NULL || name == NULL || regtype == NULL || callback == NULL) {
        status = kDNSSDAdvertisingProxyStatus_Invalid;
        goto out;
    }
    run_context_t queue = clientq;
    if (queue == NULL) {
        queue = dispatch_get_main_queue();
    }
    subscription = advertising_proxy_subscription_create();
    if (subscription == NULL) {
        status = kDNSSDAdvertisingProxyStatus_NoMemory;
        goto out;
    }
    local_instance = adv_instance_state_create(name, regtype, "local");
    if (local_instance == NULL) {
        status = kDNSSDAdvertisingProxyStatus_NoMemory;
        goto out;
    }
    adv_instance_state_subscriber_add(local_instance, subscription);
    subscription->local_instance = local_instance;
    RETAIN_HERE(subscription->local_instance, adv_instance_state);

    subscription->instance_callback = callback;
    subscription->context = context;

    DNSServiceErrorType error = DNSServiceResolve(&local_instance->resolve_ref, 0, kDNSServiceInterfaceIndexAny,
                                                  name, regtype, domain == NULL ? "local" : domain,
                                                  advertising_proxy_resolve_callback, local_instance);
    if (error == kDNSServiceErr_NoError) {
        error = DNSServiceSetDispatchQueue(local_instance->resolve_ref, queue);
        if (error != kDNSServiceErr_NoError) {
            status = kDNSSDAdvertisingProxyStatus_UnknownErr;
            goto out;
        }
    }
    if (error != kDNSServiceErr_NoError) {
        ERROR("resolve for %{private}s on service %{public}s in the default domains failed with %d",
              name, regtype, error);
        status = kDNSSDAdvertisingProxyStatus_UnknownErr;
        goto out;
    }
    RETAIN_HERE(local_instance, adv_instance_state); // for callback

    if (domain == NULL) {
        push_instance = adv_instance_state_create(name, regtype, "default.service.arpa");
        if (push_instance == NULL) {
            status = kDNSSDAdvertisingProxyStatus_NoMemory;
            goto out;
        }
        adv_instance_state_subscriber_add(push_instance, subscription);
        subscription->push_instance = push_instance;
        RETAIN_HERE(subscription->push_instance, adv_instance_state);

        error = DNSServiceResolve(&push_instance->resolve_ref, 0, kDNSServiceInterfaceIndexAny,
                                  name, regtype, "default.service.arpa",
                                  advertising_proxy_resolve_callback, push_instance);
        if (error == kDNSServiceErr_NoError) {
            error = DNSServiceSetDispatchQueue(push_instance->resolve_ref, queue);
            if (error != kDNSServiceErr_NoError) {
                status = kDNSSDAdvertisingProxyStatus_UnknownErr;
                goto out;
            }
            RETAIN_HERE(push_instance, adv_instance_state); // for callback
        }
    }

    if (error != kDNSServiceErr_NoError) {
        ERROR("resolve for %{private}s on service %{public}s in the push domain failed with %d", name, regtype, error);
        status = kDNSSDAdvertisingProxyStatus_UnknownErr;
        goto out;
    }
    *subscription_ret = subscription;
    subscription = NULL;
    push_instance = NULL;
    local_instance = NULL;

out:
    if (push_instance != NULL) {
        adv_instance_state_cancel(push_instance);
        RELEASE_HERE(push_instance, adv_instance_state);
    }
    if (local_instance != NULL) {
        adv_instance_state_cancel(local_instance);
        RELEASE_HERE(local_instance, adv_instance_state);
    }
    if (subscription != NULL) {
        advertising_proxy_subscription_cancel(subscription);
        RELEASE_HERE(subscription, advertising_proxy_subscription);
    }
    return status;
}

advertising_proxy_error_type
advertising_proxy_get_addresses(advertising_proxy_subscription_t **subscription_ret, run_context_t clientq,
                                const char *name, advertising_proxy_address_reply callback, void *context)
{
    (void)subscription_ret;
    (void)clientq;
    (void)name;
    (void)callback;
    (void)context;
    return kDNSSDAdvertisingProxyStatus_NoError;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
