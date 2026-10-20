// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef INFILTRATR_RESOURCE_CLOCK_CIVIL_H
#define INFILTRATR_RESOURCE_CLOCK_CIVIL_H

/*
 * Shared source, independently compiled into the consuming clock .so.
 * Offsets are seconds east of UTC. Reduce them modulo a day before
 * multiplication to protect int64 intermediates at all representable inputs.
 * A positive modulus is an invariant of every call.
 */
#include <stdint.h>

#define CLOCK_CIVIL_DAY_US INT64_C(86400000000)
#define CLOCK_CIVIL_SECOND_US INT64_C(1000000)

static inline int64_t clock_civil_modulo(int64_t value, int64_t modulus)
{
    const int64_t remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

static inline uint64_t clock_civil_local_phase(int64_t instant,
                                                int32_t offset_seconds)
{
    const int64_t utc_phase = clock_civil_modulo(instant, CLOCK_CIVIL_DAY_US);
    const int64_t shifted = utc_phase +
        (int64_t)(offset_seconds % INT32_C(86400)) * CLOCK_CIVIL_SECOND_US;
    return (uint64_t)clock_civil_modulo(shifted, CLOCK_CIVIL_DAY_US);
}

#endif
