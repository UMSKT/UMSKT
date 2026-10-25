/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Walks the keys.json object shape. Strings are spans into the source buffer.
 */

#ifndef UMSKT_WIN_JSON_H
#define UMSKT_WIN_JSON_H

struct JsonSpan {
    const char *p;
    int n;
};

/* Find a direct child of an object. `object` includes the surrounding braces. */
int json_object_find(const char *object, int length, const char *key, JsonSpan *value);

/* If value is a JSON string, point at the unquoted bytes. They are not NUL-terminated. */
int json_unquote(const JsonSpan *value, const char **text, int *length);

/* Iterate direct children. *cursor starts at 0 and is updated. Returns 0 when finished. */
int json_object_next(const char *object, int length, int *cursor, const char **key, int *key_length, JsonSpan *value);

/* Iterate array elements. *cursor starts at 0. */
int json_array_next(const char *array, int length, int *cursor, JsonSpan *value);

#endif
