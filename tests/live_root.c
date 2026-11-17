#include "collector.h"

#include <stddef.h>

extern int answer(void);

int main(void)
{
    void *kept;
    void *next;

    if (answer() == 0)
        return 1;
    if (smallcpp_safepoint_count() < 2)
        return 2;
    kept = smallcpp_safepoint_result(0);
    next = smallcpp_safepoint_result(1);
    if (kept == NULL || next == NULL || kept == next)
        return 3;
    if (!smallcpp_safepoint_traced(1, kept))
        return 4;
    return 0;
}
