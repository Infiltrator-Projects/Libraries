// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
/**
 * @file temporal_resource.c
 * @brief Cross-platform temporal resource directory discovery.
 */
#include "infiltratr/temporal_resource.h"
#include "string_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <wchar.h>
#include <windows.h>
#else
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#endif

typedef struct TemporalCandidate {
    char *name;
    InfiltratrTemporalResourceForm form;
} TemporalCandidate;

typedef struct TemporalCandidateList {
    TemporalCandidate *items;
    size_t count;
    size_t capacity;
} TemporalCandidateList;

static bool resource_kind_valid(InfiltratrTemporalResourceKind kind)
{
    return kind == INFILTRATR_TEMPORAL_RESOURCE_CALENDAR ||
           kind == INFILTRATR_TEMPORAL_RESOURCE_CLOCK;
}

static char *join_path(const char *directory, const char *name)
{
    size_t directory_length;
    size_t name_length;
    bool needs_separator;
    size_t total;
    char *path;
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif

    if (directory == NULL || directory[0] == '\0' ||
        name == NULL || name[0] == '\0') {
        return NULL;
    }

    directory_length = strlen(directory);
    name_length = strlen(name);
    needs_separator = directory[directory_length - 1U] != '/' &&
                      directory[directory_length - 1U] != '\\';
    if (directory_length > SIZE_MAX - name_length - 2U) return NULL;
    total = directory_length + name_length + (needs_separator ? 2U : 1U);
    path = malloc(total);
    if (path == NULL) return NULL;

    memcpy(path, directory, directory_length);
    if (needs_separator) path[directory_length++] = separator;
    memcpy(path + directory_length, name, name_length + 1U);
    return path;
}

static void candidate_list_clear(TemporalCandidateList *list)
{
    size_t index;

    if (list == NULL) return;
    for (index = 0U; index < list->count; ++index) free(list->items[index].name);
    free(list->items);
    list->items = NULL;
    list->count = 0U;
    list->capacity = 0U;
}

static bool candidate_list_add(TemporalCandidateList *list,
                               const char *name,
                               InfiltratrTemporalResourceForm form)
{
    TemporalCandidate *grown;
    size_t next_capacity;
    char *owned_name;

    if (list == NULL || name == NULL || name[0] == '\0') return false;
    if (list->count == list->capacity) {
        next_capacity = list->capacity == 0U ? 8U : list->capacity * 2U;
        if (next_capacity < list->capacity ||
            next_capacity > SIZE_MAX / sizeof(*grown)) {
            return false;
        }
        grown = realloc(list->items, next_capacity * sizeof(*grown));
        if (grown == NULL) return false;
        list->items = grown;
        list->capacity = next_capacity;
    }

    owned_name = infiltratr_private_copy_text(name);
    if (owned_name == NULL) return false;
    list->items[list->count].name = owned_name;
    list->items[list->count].form = form;
    list->count++;
    return true;
}

static int candidate_compare(const void *left, const void *right)
{
    const TemporalCandidate *left_candidate = left;
    const TemporalCandidate *right_candidate = right;
    return strcmp(left_candidate->name, right_candidate->name);
}

#ifdef _WIN32
static wchar_t *utf8_to_wide(const char *text)
{
    int characters;
    wchar_t *wide;

    if (text == NULL || text[0] == '\0') return NULL;
    characters = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                     text, -1, NULL, 0);
    if (characters <= 0 || (size_t)characters > SIZE_MAX / sizeof(*wide))
        return NULL;
    wide = malloc((size_t)characters * sizeof(*wide));
    if (wide == NULL) return NULL;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                            text, -1, wide, characters) != characters) {
        free(wide);
        return NULL;
    }
    return wide;
}

static char *wide_to_utf8(const wchar_t *text)
{
    int bytes;
    char *utf8;

    if (text == NULL || text[0] == L'\0') return NULL;
    bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                               text, -1, NULL, 0, NULL, NULL);
    if (bytes <= 0) return NULL;
    utf8 = malloc((size_t)bytes);
    if (utf8 == NULL) return NULL;
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            text, -1, utf8, bytes, NULL, NULL) != bytes) {
        free(utf8);
        return NULL;
    }
    return utf8;
}

static bool collect_candidates(const char *directory,
                               TemporalCandidateList *list)
{
    wchar_t *wide_directory;
    wchar_t *pattern;
    size_t directory_length;
    bool needs_separator;
    WIN32_FIND_DATAW data;
    HANDLE find;
    bool ok = true;

    wide_directory = utf8_to_wide(directory);
    if (wide_directory == NULL) return false;
    directory_length = wcslen(wide_directory);
    needs_separator = wide_directory[directory_length - 1U] != L'/' &&
                      wide_directory[directory_length - 1U] != L'\\';
    if (directory_length > SIZE_MAX - (needs_separator ? 3U : 2U) ||
        directory_length + (needs_separator ? 3U : 2U) >
            SIZE_MAX / sizeof(*pattern)) {
        free(wide_directory);
        return false;
    }
    pattern = malloc((directory_length + (needs_separator ? 3U : 2U)) *
                     sizeof(*pattern));
    if (pattern == NULL) {
        free(wide_directory);
        return false;
    }
    memcpy(pattern, wide_directory, directory_length * sizeof(*pattern));
    if (needs_separator) pattern[directory_length++] = L'\\';
    pattern[directory_length++] = L'*';
    pattern[directory_length] = L'\0';
    free(wide_directory);

    find = FindFirstFileW(pattern, &data);
    free(pattern);
    if (find == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }

    do {
        char *name;
        InfiltratrTemporalResourceForm form;

        if (data.cFileName[0] == L'.' ||
            (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U ||
            (data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0U) {
            continue;
        }
        name = wide_to_utf8(data.cFileName);
        if (name == NULL) {
            ok = false;
            break;
        }
        form = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U
            ? INFILTRATR_TEMPORAL_RESOURCE_DIRECTORY
            : INFILTRATR_TEMPORAL_RESOURCE_FILE;
        if (!candidate_list_add(list, name, form)) ok = false;
        free(name);
        if (!ok) break;
    } while (FindNextFileW(find, &data) != 0);

    if (ok && GetLastError() != ERROR_NO_MORE_FILES) ok = false;
    FindClose(find);
    return ok;
}
#else
static bool collect_candidates(const char *directory,
                               TemporalCandidateList *list)
{
    DIR *stream;
    struct dirent *entry;

    errno = 0;
    stream = opendir(directory);
    if (stream == NULL) return errno == ENOENT;

    for (;;) {
        char *path;
        struct stat status;
        InfiltratrTemporalResourceForm form;

        errno = 0;
        entry = readdir(stream);
        if (entry == NULL) {
            const int read_error = errno;
            if (closedir(stream) != 0) return false;
            return read_error == 0;
        }
        if (entry->d_name[0] == '.') continue;
        path = join_path(directory, entry->d_name);
        if (path == NULL) {
            closedir(stream);
            return false;
        }
        if (lstat(path, &status) != 0) {
            free(path);
            continue;
        }
        free(path);
        if (S_ISDIR(status.st_mode)) {
            form = INFILTRATR_TEMPORAL_RESOURCE_DIRECTORY;
        } else if (S_ISREG(status.st_mode)) {
            form = INFILTRATR_TEMPORAL_RESOURCE_FILE;
        } else {
            continue;
        }
        if (!candidate_list_add(list, entry->d_name, form)) {
            closedir(stream);
            return false;
        }
    }
}
#endif

bool infiltratr_temporal_resource_scan_directory(
    const char *directory,
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalResourceVisitor visitor,
    void *user_data,
    size_t *visited)
{
    TemporalCandidateList candidates = {0};
    size_t emitted = 0U;
    size_t index;

    if (directory == NULL || directory[0] == '\0' ||
        !resource_kind_valid(kind)) {
        return false;
    }
    if (!collect_candidates(directory, &candidates)) {
        candidate_list_clear(&candidates);
        return false;
    }
    if (candidates.count > 1U) {
        qsort(candidates.items, candidates.count, sizeof(*candidates.items),
              candidate_compare);
    }

    for (index = 0U; index < candidates.count; ++index) {
        char *path = join_path(directory, candidates.items[index].name);
        InfiltratrTemporalResourceInfo info;

        if (path == NULL) {
            candidate_list_clear(&candidates);
            return false;
        }
        info.struct_size = sizeof(info);
        info.abi_version = INFILTRATR_TEMPORAL_RESOURCE_INFO_ABI;
        info.kind = kind;
        info.form = candidates.items[index].form;
        info.name = candidates.items[index].name;
        info.path = path;
        if (visitor != NULL && !visitor(&info, user_data)) {
            free(path);
            candidate_list_clear(&candidates);
            return false;
        }
        free(path);
        emitted++;
    }

    candidate_list_clear(&candidates);
    if (visited != NULL) *visited = emitted;
    return true;
}

bool infiltratr_temporal_resource_scan_root(
    const char *root,
    InfiltratrTemporalResourceVisitor visitor,
    void *user_data,
    size_t *visited)
{
    char *calendars;
    char *clocks;
    size_t calendar_count = 0U;
    size_t clock_count = 0U;

    if (root == NULL || root[0] == '\0') return false;
    calendars = join_path(root, "calendars");
    clocks = join_path(root, "clocks");
    if (calendars == NULL || clocks == NULL) {
        free(calendars);
        free(clocks);
        return false;
    }

    if (!infiltratr_temporal_resource_scan_directory(
            calendars, INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
            visitor, user_data, &calendar_count) ||
        !infiltratr_temporal_resource_scan_directory(
            clocks, INFILTRATR_TEMPORAL_RESOURCE_CLOCK,
            visitor, user_data, &clock_count)) {
        free(calendars);
        free(clocks);
        return false;
    }
    free(calendars);
    free(clocks);
    if (visited != NULL) *visited = calendar_count + clock_count;
    return true;
}
