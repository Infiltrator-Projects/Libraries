// SPDX-License-Identifier: GPL-3.0-or-later
/* Private, dependency-free owning text copy for Common's C adapters. */
#ifndef INFILTRATR_STRING_INTERNAL_H
#define INFILTRATR_STRING_INTERNAL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline char *infiltratr_private_copy_text(const char *text)
{
    size_t length;
    char *copy;

    if (text == NULL) return NULL;
    length = strlen(text);
    if (length == SIZE_MAX) return NULL;
    copy = malloc(length + 1U);
    if (copy == NULL) return NULL;
    memcpy(copy, text, length + 1U);
    return copy;
}

#endif
