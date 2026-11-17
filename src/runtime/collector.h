#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate bytes * count. Traces the caller's safepoint first. delete does not call this. */
void *smallcpp_alloc(uint64_t bytes, uint64_t count);

/* Safepoints, in the order smallcpp_alloc ran. Used to check a traced root. */
int smallcpp_safepoint_count(void);
void *smallcpp_safepoint_result(int index);
int smallcpp_safepoint_traced(int index, void *ptr);

#ifdef __cplusplus
}
#endif
