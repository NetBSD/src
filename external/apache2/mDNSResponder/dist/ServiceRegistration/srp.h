/* srp.h
 *
 * Copyright (c) 2018-2024 Apple Inc. All rights reserved.
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
 * Service Registration Protocol common definitions
 */

#ifndef __SRP_H
#define __SRP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef THREAD_DEVKIT_ADK
#include <netinet/in.h>
#endif
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#ifdef POSIX_BUILD
#include <limits.h>
#include <sys/param.h>
#endif
#ifdef MALLOC_DEBUG_LOGGING
#  define MDNS_NO_STRICT 1
#endif

#include "srp-features.h"           // for feature flags


#ifdef __clang__
#define NULLABLE _Nullable
#define NONNULL _Nonnull
#define UNUSED __unused
#else
#define NULLABLE
#define NONNULL
#define UNUSED __attribute__((unused))
#ifdef POSIX_BUILD
#else
#define SRP_CRYPTO_MBEDTLS 1
#endif // POSIX_BUILD
#endif

#define INT64_HEX_STRING_MAX 17     // Maximum size of an int64_t printed as hex, including NUL termination

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
//======================================================================================================================

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "srp-log.h"                // For log functions

#define SRP_OBJ_REF_COUNT_LIMIT    10000

#ifdef __clang__
#define FILE_TRIM(x) (strrchr(x, '/') + 1)
#else
#define FILE_TRIM(x) (x)
#endif

#ifdef THREAD_DEVKIT_ADK
#define FINALIZED(x)
#define CREATED(x)
#else
#define FINALIZED(x) ((x)++)
#define CREATED(x) ((x)++)
#endif // THREAD_DEVKIT_ADK

#ifdef DEBUG_VERBOSE
#ifdef __clang_analyzer__
#define RELEASE_BASE(x, object_type, file, line) \
    object_type ## _finalize(x)
#else
#define RELEASE_BASE(x, object_type, file, line) do {                             \
        if ((x) != NULL) {                                                        \
            if ((x)->ref_count == 0) {                                            \
                FAULT("ALLOC: release after finalize at %2.2d: %p (%10s): %s:%d", \
                      (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);   \
                abort();                                                          \
            } else if ((x)->ref_count > SRP_OBJ_REF_COUNT_LIMIT) {                \
                FAULT("ALLOC: release at %2.2d: %p (%10s): %s:%d",                \
                      (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);   \
                abort();                                                          \
            } else {                                                              \
                INFO("ALLOC: release at %2.2d: %p (%10s): %s:%d",                 \
                     (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);    \
                --(x)->ref_count;                                                 \
                if ((x)->ref_count == 0) {                                        \
                    INFO("ALLOC:      finalize: %p (%10s): %s:%d",                \
                         (void *)(x), # x, FILE_TRIM(file), line);                \
                    FINALIZED(object_type##_finalized);                           \
                    object_type##_finalize(x);                                    \
                }                                                                 \
            }                                                                     \
        }                                                                         \
    } while (0)

#endif // __clang_analyzer__
#define RETAIN_BASE(x, object_type, file, line) do {                              \
        if ((x) != NULL) {                                                        \
            INFO("ALLOC:  retain at %2.2d: %p (%10s): %s:%d",                     \
                 (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);        \
            if ((x)->ref_count == 0) {                                            \
               CREATED(object_type##_created);                                    \
            }                                                                     \
            ++((x)->ref_count);                                                   \
            if ((x)->ref_count > SRP_OBJ_REF_COUNT_LIMIT) {                       \
                FAULT("ALLOC: retain at %2.2d: %p (%10s): %s:%d",                 \
                      (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);   \
                abort();                                                          \
            }                                                                     \
        }                                                                         \
    } while (0)
#define RELEASE(x, object_type) RELEASE_BASE(x, object_type, file, line)
#define RETAIN(x, object_type) RETAIN_BASE(x, object_type, file, line)
#define RELEASE_HERE(x, object_type) RELEASE_BASE(x, object_type, __FILE__, __LINE__)
#define RETAIN_HERE(x, object_type) RETAIN_BASE(x, object_type, __FILE__, __LINE__)
#else // DEBUG_VERBOSE
#ifdef __clang_analyzer__
#define RELEASE(x, object_type) object_type ## _finalize(x)
#define RELEASE_HERE(x, object_type) object_type ## _finalize(x)
#define RETAIN(x, object_type)
#define RETAIN_HERE(x, object_tyoe)
#else
#define RELEASE(x, object_type) do {                                          \
        if ((x)->ref_count == 0) {                                            \
            FAULT("ALLOC: release after finalize at %2.2d: %p (%10s): %s:%d", \
                  (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);   \
            abort();                                                          \
        }                                                                     \
        if ((x)->ref_count > SRP_OBJ_REF_COUNT_LIMIT) {                       \
            FAULT("ALLOC: release at %2.2d: %p (%10s): %s:%d",                \
                  (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);   \
            abort();                                                          \
        }                                                                     \
        if (--(x)->ref_count == 0) {                                          \
            FINALIZED(object_type##_finalized);                               \
            object_type ## _finalize(x);                                      \
            (void)file; (void)line;                                           \
        }                                                                     \
    } while (0)
#define RETAIN(x, object_type) do {                                           \
        (x)->ref_count++;                                                     \
        if (--(x)->ref_count == 0) {                                          \
            CREATED(object_type##_created);                                   \
        }                                                                     \
        if ((x)->ref_count > SRP_OBJ_REF_COUNT_LIMIT) {                       \
            FAULT("ALLOC: retain at %2.2d: %p (%10s): %s:%d",                 \
                  (x)->ref_count, (void *)(x), # x, FILE_TRIM(file), line);   \
            abort();                                                          \
        }                                                                     \
    } while (0)
#define RELEASE_HERE(x, object_type) do {                                     \
        if ((x)->ref_count == 0) {                                            \
            FAULT("ALLOC: release after finalize at %2.2d: %p (%10s)",        \
                  (x)->ref_count, (void *)(x), # x);                          \
            abort();                                                          \
        }                                                                     \
        if ((x)->ref_count > SRP_OBJ_REF_COUNT_LIMIT) {                       \
            FAULT("ALLOC: release at %2.2d: %p (%10s)",                       \
                  (x)->ref_count, (void *)(x), # x);                          \
            abort();                                                          \
        }                                                                     \
        if (--(x)->ref_count == 0) {                                          \
            FINALIZED(object_type##_finalized);                               \
            object_type ## _finalize(x);                                      \
        }                                                                     \
    } while (0)
#define RETAIN_HERE(x, object_type) do {                                      \
        (x)->ref_count++;                                                     \
        if (--(x)->ref_count == 0) {                                          \
            CREATED(object_type##_created);                                   \
        }                                                                     \
        if ((x)->ref_count > SRP_OBJ_REF_COUNT_LIMIT) {                       \
            FAULT("ALLOC: retain at %2.2d: %p (%10s)",                        \
                  (x)->ref_count, (void *)(x), # x);                          \
            abort();                                                          \
        }                                                                     \
    } while (0)
#endif
#endif // DEBUG_VERBOSE

#define THREAD_ENTERPRISE_NUMBER ((uint64_t)44970)
#define THREAD_SRP_SERVER_ANYCAST_OPTION 0x5c
#define THREAD_SRP_SERVER_OPTION 0x5d
#define THREAD_PREF_ID_OPTION    0x9d

#define IS_SRP_SERVICE(service) \
    ((cti_service)->enterprise_number == THREAD_ENTERPRISE_NUMBER &&    \
     (cti_service)->service_type == THREAD_SRP_SERVER_OPTION &&         \
     (cti_service)->service_version == 1 &&                             \
     (cti_service)->server_length == 18)
#define IS_SRP_ANYCAST_SERVICE(service) \
    ((cti_service)->enterprise_number == THREAD_ENTERPRISE_NUMBER &&    \
     (cti_service)->service_type == THREAD_SRP_SERVER_ANYCAST_OPTION &&         \
     (cti_service)->service_version == 1 &&                             \
     (cti_service)->service_length == 2)
#define IS_PREF_ID_SERVICE(service) \
    ((cti_service)->enterprise_number == THREAD_ENTERPRISE_NUMBER &&    \
     (cti_service)->service_type == THREAD_PREF_ID_OPTION &&            \
     (cti_service)->service_version == 1 &&                             \
     (cti_service)->server_length == 9)

#ifdef MALLOC_DEBUG_LOGGING
void *debug_malloc(size_t len, const char *file, int line);
void *debug_calloc(size_t count, size_t len, const char *file, int line);
char *debug_strdup(const char *s, const char *file, int line);
void debug_free(void *p, const char *file, int line);

#define malloc(x) debug_malloc(x, __FILE__, __LINE__)
#define calloc(c, y) debug_calloc(c, y, __FILE__, __LINE__)
#define strdup(s) debug_strdup(s, __FILE__, __LINE__)
#define free(p) debug_free(p, __FILE__, __LINE__)
#endif

typedef struct srp_key srp_key_t;

// This function compares two IPv6 prefixes, up to the specified prefix length (in bytes).
// return: -1 if prefix_a < prefix_b
//          0 if prefix_a == prefix_b
//          1 if prefix_a > prefix_b.
static inline int
in6prefix_compare(const struct in6_addr *prefix_a, const struct in6_addr *prefix_b, size_t len)
{
    return memcmp(prefix_a, prefix_b, len);
}

// This function compares two full IPv6 addresses.
// return: -1 if addr_a < addr_b
//          0 if addr_a == addr_b
//          1 if addr_a > addr_b.
static inline int
in6addr_compare(const struct in6_addr *addr_a, const struct in6_addr *addr_b)
{
    return in6prefix_compare(addr_a, addr_b, sizeof (*addr_a));
}

// This function copies the data into a, up to len bytes or sizeof(*a), whichever is less.
// if there are uninitialized bytes remaining in a, sets those to zero.
static inline void
in6prefix_copy_from_data(struct in6_addr *prefix, const uint8_t *data, size_t len)
{
    size_t copy_len = sizeof(*prefix) < len ? sizeof(*prefix): len;
    if (copy_len > 0) {
        memcpy(prefix, data, copy_len);
    }
    if (copy_len != sizeof(*prefix)) {
        memset((char *)prefix + copy_len, 0, sizeof(*prefix) - copy_len);
    }
}

// This function copies prefix src, into prefix dst, up to len bytes.
static inline void
in6prefix_copy(struct in6_addr *dst, const struct in6_addr *src, size_t len)
{
    in6prefix_copy_from_data(dst, (const uint8_t*)src, len);
}

// This function copies full IPv6 address src into dst.
static inline void
in6addr_copy(struct in6_addr *dst, const struct in6_addr *src)
{
    memcpy(dst, src, sizeof(*dst));
}

// This function zeros the full IPv6 address
static inline void
in6addr_zero(struct in6_addr *addr)
{
    memset(addr, 0, sizeof(*addr));
}

// Returns true if this is a Thread mesh-local anycast address.
extern const uint8_t thread_anycast_preamble[7];
extern const uint8_t thread_rloc_preamble[6];

static inline bool
is_thread_mesh_anycast_address(const struct in6_addr *addr)
{
    // Thread 1.3.0 section 5.2.2.2 Anycast Locator (ALOC)
    if (!memcmp(&addr->s6_addr[8], thread_anycast_preamble, sizeof(thread_anycast_preamble))) {
        return true;
    }
    return false;
}

static inline bool
is_thread_mesh_rloc_address(const struct in6_addr *addr)
{
    // Thread 1.3.0 section 5.2.2.1 Routing Locator (RLOC)
    if (!memcmp(&addr->s6_addr[8], thread_rloc_preamble, sizeof(thread_rloc_preamble))) {
        return true;
    }
    return false;
}

static inline bool
is_thread_mesh_synthetic_address(const struct in6_addr *addr)
{
    return is_thread_mesh_anycast_address(addr) || is_thread_mesh_rloc_address(addr);
}

static inline bool
is_thread_mesh_synthetic_or_link_local(const struct in6_addr *addr)
{
    return (is_thread_mesh_anycast_address(addr) || is_thread_mesh_rloc_address(addr) ||
            (addr->s6_addr[0] == 0xfe && (addr->s6_addr[1] & 0xc0) == 0x80));
}

#ifndef _SRP_STRICT_DISPOSE_TEMPLATE
    #define _SRP_STRICT_DISPOSE_TEMPLATE(PTR, FUNCTION) \
        do {                                            \
            if (*(PTR) != NULL) {                       \
                FUNCTION(*PTR);                         \
                *(PTR) = NULL;                          \
            }                                           \
        } while(0)
#endif

#ifndef DNSServiceRefSourceForget
    #define DNSServiceRefSourceForget(PTR) _SRP_STRICT_DISPOSE_TEMPLATE(PTR, DNSServiceRefDeallocate)
#endif


/*!
 *  @brief
 *      Check the required condition, if the required condition is not met go to the label specified.
 *
 *  @param ASSERTION
 *      The condition that must be met before continue.
 *
 *  @param EXCEPTION_LABEL
 *      The label to go to when the required condition ASSERTION is not met.
 *
 *  @param ACTION
 *      The extra action to take before go to the EXCEPTION_LABEL label when ASSERTION is not met.
 *
 *  @discussion
 *      Example:
 *      require_action_quiet(
 *          foo == NULL, // required to be true
 *          exit, // if not met goto label
 *          ret = -1;  ERROR("foo should not be NULL") // before exiting
 *      ) ;
 */
#ifndef require_action_quiet
    #define require_action_quiet(ASSERTION, EXCEPTION_LABEL, ACTION)    \
        do {                                                            \
            if (__builtin_expect(!(ASSERTION), 0))                      \
            {                                                           \
                {                                                       \
                    ACTION;                                             \
                }                                                       \
                goto EXCEPTION_LABEL;                                   \
            }                                                           \
        } while(0)
#endif // #ifndef require_action

#ifndef require_quiet
    #define require_quiet(ASSERTION, EXCEPTION_LABEL)                   \
        do {                                                            \
            if (__builtin_expect(!(ASSERTION), 0))                      \
            {                                                           \
                goto EXCEPTION_LABEL;                                   \
            }                                                           \
        } while(0)
#endif // #ifndef require_action

/*!
 *  @brief
 *      Check the required condition, if the required condition is not met go to the label specified.
 *
 *  @param ASSERTION
 *      The condition that must be met before continue.
 *
 *  @param EXCEPTION_LABEL
 *      The label to go to when the required condition ASSERTION is not met.
 *
 *  @param ACTION
 *      The extra action to take before go to the EXCEPTION_LABEL label when ASSERTION is not met.
 *
 *  @discussion
 *      Example:
 *      require_action(
 *          foo == NULL, // required to be true
 *          exit, // if not met goto label
 *          ret = -1;  ERROR("foo should not be NULL") // before exiting
 *      ) ;
 */
#ifndef require_action
    #define require_action(ASSERTION, EXCEPTION_LABEL, ACTION)    \
        do {                                                            \
            if (__builtin_expect(!(ASSERTION), 0))                      \
            {                                                           \
                {                                                       \
                    ACTION;                                             \
                }                                                       \
                goto EXCEPTION_LABEL;                                   \
            }                                                           \
        } while(0)
#endif // #ifndef require_action

/*!
 *  @brief
 *      Check the required condition, if the required condition is not met, do the ACTION. It is usually used as DEBUG macro.
 *
 *  @param ASSERTION
 *      The condition that must be met before continue.
 *
 *  @param ACTION
 *      The extra action to take when ASSERTION is not met.
 *
 *  @discussion
 *      Example:
 *      verify_action(
 *          foo == NULL, // required to be true
 *          ERROR("foo should not be NULL")  // action to take if required is false
 *      ) ;
 */
#undef verify_action
#define verify_action(ASSERTION, ACTION)                                \
    if (__builtin_expect(!(ASSERTION), 0)) {                            \
        ACTION;                                                         \
    }                                                                   \
    else do {} while (0)

// Print true or false based on boolean value:
    static inline const char *bool_str(bool tf) {
        if (tf) return "true";
        return "false";
    }


#ifdef __cplusplus
} // extern "C"
#endif

#ifndef THREAD_DEVKIT_ADK
// Object type external definitions
#define OBJECT_TYPE(x) extern int x##_created, x##_finalized, old_##x##_created, old_##x##_finalized;
#include "object-types.h"
#endif // !THREAD_DEVKIT_ADK

#endif // __SRP_H

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
