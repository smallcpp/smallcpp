#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate bytes * count. Traces the caller's safepoint first, then reclaims
 * objects that trace cannot reach. delete does not call this and does not free. */
void *smallcpp_alloc(uint64_t bytes, uint64_t count);

/* Remember the destructor for an object from new. A null destructor means none. */
void smallcpp_set_dtor(void *ptr, void (*dtor)(void *));

/* The destructor ran. The collector must not call it when the object is reclaimed. */
void smallcpp_dtor_done(void *ptr);

/* Safepoints, in the order smallcpp_alloc ran. Used to check a traced root. */
int smallcpp_safepoint_count(void);
void *smallcpp_safepoint_result(int index);
int smallcpp_safepoint_traced(int index, void *ptr);

#ifdef __cplusplus
}
#endif
