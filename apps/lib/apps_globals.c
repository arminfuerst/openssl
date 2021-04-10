/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include "apps_os_wrapper.h"
#include "apps_globals.h"
#include "fmt.h"
#include "opt.h"
#include <openssl/err.h>
#include <ctype.h>
#include <string.h>

void cleanse(char *str)
{
    if (str != NULL)
        OPENSSL_cleanse(str, strlen(str));
}

int set_ext_copy(int *copy_type, const char *arg)
{
    if (app_strcasecmp(arg, "none") == 0)
        *copy_type = EXT_COPY_NONE;
    else if (app_strcasecmp(arg, "copy") == 0)
        *copy_type = EXT_COPY_ADD;
    else if (app_strcasecmp(arg, "copyall") == 0)
        *copy_type = EXT_COPY_ALL;
    else
        return 0;
    return 1;
}

int parse_yesno(const char *str, int def)
{
    if (str) {
        switch (*str) {
        case 'f':              /* false */
        case 'F':              /* FALSE */
        case 'n':              /* no */
        case 'N':              /* NO */
        case '0':              /* 0 */
            return 0;
        case 't':              /* true */
        case 'T':              /* TRUE */
        case 'y':              /* yes */
        case 'Y':              /* YES */
        case '1':              /* 1 */
            return 1;
        }
    }
    return def;
}

void make_uppercase(char *string)
{
    int i;

    for (i = 0; string[i] != '\0'; i++)
        string[i] = (char)toupper((unsigned char)string[i]);
}

int app_isdir(const char *name)
{
    return opt_isdir(name);
}

const char *modestr(char mode, int format)
{
    OPENSSL_assert(mode == 'a' || mode == 'r' || mode == 'w');

    switch (mode) {
    case 'a':
        return FMT_istext(format) ? "a" : "ab";
    case 'r':
        return FMT_istext(format) ? "r" : "rb";
    case 'w':
        return FMT_istext(format) ? "w" : "wb";
    }
    /* The assert above should make sure we never reach this point */
    return NULL;
}

const char *modeverb(char mode)
{
    switch (mode) {
    case 'a':
        return "appending";
    case 'r':
        return "reading";
    case 'w':
        return "writing";
    }
    return "(doing something)";
}

void app_bail_out(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    BIO_vprintf(bio_err, fmt, args);
    va_end(args);
    ERR_print_errors(bio_err);
    exit(1);
}

void *app_malloc(size_t sz, const char *what)
{
    void *vp = OPENSSL_malloc(sz);

    if (vp == NULL)
        app_bail_out("%s: Could not allocate %zu bytes for %s\n",
                     opt_getprog(), sz, what);
    return vp;
}

int int_2_size_t(int src, size_t *dst)
{
    if (src >= 0) {
        *dst = (size_t)src;
        return 1;
    }
    return 0;
}

int size_t_2_int(size_t src, int *dst)
{
    if (src <= INT_MAX) {
        *dst = (int)src;
        return 1;
    }
    return 0;
}

int str_2_int(const char *src, int *dst)
{
    char *end;
    long sl;

    errno = 0;

    sl = strtol(src, &end, 10);

    if (end == src) {
        /* string was not a decimal number */
        return 0;
    } else if ('\0' != *end) {
        /* extra characters ad the end */
        return 0;
    } else if ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) {
        /* out of range */
        return 0;
    } else if (sl > INT_MAX) {
        /* too big for integer */
        return 0;
    } else if (sl < INT_MIN) {
        /* too small for integer */
        return 0;
    }
    *dst = (int)sl;
    return 1;
}

int str_2_size_t(const char *src, size_t *dst)
{
    char *end;
    long long sl;

    errno = 0;

    sl = strtoll(src, &end, 10);

    if (end == src) {
        /* string was not a decimal number */
        return 0;
    } else if ('\0' != *end) {
        /* extra characters ad the end */
        return 0;
    } else if ((LLONG_MIN == sl || LLONG_MAX == sl) && ERANGE == errno) {
        /* out of range */
        return 0;
    } else if ((sl > 0) && ((unsigned long long)sl > SIZE_MAX)) {
        /* too big for integer */
        return 0;
    } else if (sl < 0) {
        /* too small for size_t */
        return 0;
    }
    *dst = (int)sl;
    return 1;
}

