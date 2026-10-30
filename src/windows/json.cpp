/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "json.h"

struct JsonCursor {
    const char *begin;
    const char *p;
    const char *end;
};

static void skip_ws(JsonCursor *j) {
    while (j->p < j->end) {
        char ch = *j->p;
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            break;
        }
        j->p++;
    }
}

static int skip_string(JsonCursor *j) {
    if (j->p >= j->end || *j->p != '"') {
        return 0;
    }
    j->p++;
    while (j->p < j->end) {
        char ch = *j->p++;
        if (ch == '\\') {
            if (j->p < j->end) {
                j->p++;
            }
            continue;
        }
        if (ch == '"') {
            return 1;
        }
    }
    return 0;
}

static int skip_value(JsonCursor *j) {
    skip_ws(j);
    if (j->p >= j->end) {
        return 0;
    }
    if (*j->p == '"') {
        return skip_string(j);
    }
    if (*j->p == '{' || *j->p == '[') {
        char open = *j->p;
        char close = open == '{' ? '}' : ']';
        int depth = 0;
        while (j->p < j->end) {
            if (*j->p == '"') {
                if (!skip_string(j)) {
                    return 0;
                }
                continue;
            }
            if (*j->p == open) {
                depth++;
            } else if (*j->p == close) {
                depth--;
                j->p++;
                if (depth == 0) {
                    return 1;
                }
                continue;
            }
            j->p++;
        }
        return 0;
    }
    while (j->p < j->end) {
        char ch = *j->p;
        if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            break;
        }
        j->p++;
    }
    return 1;
}

static int parse_key(JsonCursor *j, const char **key, int *key_length) {
    const char *start;
    skip_ws(j);
    if (j->p >= j->end || *j->p != '"') {
        return 0;
    }
    j->p++;
    start = j->p;
    while (j->p < j->end && *j->p != '"') {
        if (*j->p == '\\') {
            return 0;
        }
        j->p++;
    }
    if (j->p >= j->end) {
        return 0;
    }
    *key = start;
    *key_length = (int)(j->p - start);
    j->p++;
    return 1;
}

static int key_eq(const char *key, int key_length, const char *wanted) {
    int i = 0;
    while (wanted[i] && i < key_length) {
        if (key[i] != wanted[i]) {
            return 0;
        }
        i++;
    }
    return wanted[i] == 0 && i == key_length;
}

int json_object_find(const char *object, int length, const char *key, JsonSpan *value) {
    JsonCursor j;
    j.begin = object;
    j.p = object;
    j.end = object + length;
    skip_ws(&j);
    if (j.p >= j.end || *j.p != '{') {
        return 0;
    }
    j.p++;
    while (j.p < j.end) {
        const char *found;
        int found_len;
        skip_ws(&j);
        if (j.p < j.end && *j.p == '}') {
            return 0;
        }
        if (!parse_key(&j, &found, &found_len)) {
            return 0;
        }
        skip_ws(&j);
        if (j.p >= j.end || *j.p != ':') {
            return 0;
        }
        j.p++;
        skip_ws(&j);
        if (key_eq(found, found_len, key)) {
            const char *start = j.p;
            if (!skip_value(&j)) {
                return 0;
            }
            value->p = start;
            value->n = (int)(j.p - start);
            return 1;
        }
        if (!skip_value(&j)) {
            return 0;
        }
        skip_ws(&j);
        if (j.p < j.end && *j.p == ',') {
            j.p++;
        }
    }
    return 0;
}

int json_unquote(const JsonSpan *value, const char **text, int *length) {
    const char *p;
    const char *end;
    if (!value || value->n < 2 || value->p[0] != '"') {
        return 0;
    }
    p = value->p + 1;
    end = value->p + value->n;
    if (end[-1] != '"') {
        return 0;
    }
    *text = p;
    *length = (int)(end - p - 1);
    return 1;
}

static int next_member(const char *text, int length, int *cursor, int array, const char **key, int *key_length, JsonSpan *value) {
    JsonCursor j;
    const char *open;
    j.begin = text;
    j.end = text + length;
    if (*cursor == 0) {
        j.p = text;
        skip_ws(&j);
        if (j.p >= j.end || *j.p != (array ? '[' : '{')) {
            return 0;
        }
        j.p++;
        *cursor = (int)(j.p - text);
    }
    j.p = text + *cursor;
    skip_ws(&j);
    if (j.p >= j.end || *j.p == (array ? ']' : '}')) {
        return 0;
    }
    if (!array) {
        if (!parse_key(&j, key, key_length)) {
            return 0;
        }
        skip_ws(&j);
        if (j.p >= j.end || *j.p != ':') {
            return 0;
        }
        j.p++;
        skip_ws(&j);
    }
    open = j.p;
    if (!skip_value(&j)) {
        return 0;
    }
    value->p = open;
    value->n = (int)(j.p - open);
    skip_ws(&j);
    if (j.p < j.end && *j.p == ',') {
        j.p++;
    }
    *cursor = (int)(j.p - text);
    return 1;
}

int json_object_next(const char *object, int length, int *cursor, const char **key, int *key_length, JsonSpan *value) {
    return next_member(object, length, cursor, 0, key, key_length, value);
}

int json_array_next(const char *array, int length, int *cursor, JsonSpan *value) {
    const char *ignored = 0;
    int ignored_len = 0;
    return next_member(array, length, cursor, 1, &ignored, &ignored_len, value);
}
