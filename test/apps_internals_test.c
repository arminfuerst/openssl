/*
 * Copyright 2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include "apps_os_wrapper.h"
#include "apps_globals.h"
#include "testutil.h"
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define binname "apps_internals_test"

static int test_app_rename(void)
{
    if (test_get_argument_count() != 3) {
        TEST_error("Usage: %s: app_rename srcfile dstfile\n", binname);
        return 0;
    }
    if (app_rename(test_get_argument(1), test_get_argument(2)) == 0) {
        return 1;
    }
    TEST_info("got error on rename: '%s'\n", strerror(errno));

    return 0;
}

static int test_app_strcasecmp(void)
{
    int rv;

    if (test_get_argument_count() != 3) {
        TEST_error("Usage: %s: app_strcasecmp string1 string2\n", binname);
        return 0;
    }
    rv = app_strcasecmp(test_get_argument(1), test_get_argument(2));
    BIO_printf(bio_out, "Result: '%i'\n", rv);
    return 1;
}

static int test_posix_file_io(void)
{
    int fd;
    int rv;
    char buf[100];

    if (test_get_argument_count() != 2) {
        TEST_error("Usage: %s: posix_file_io file_to_read\n", binname);
        return 0;
    }

    fd = app_open(test_get_argument(1), O_RDONLY, 0);
    if (fd < 0) {
        TEST_error("Error opening file '%s': %s\n", test_get_argument(1), strerror(errno));
        return 0;
    }
    rv = app_read(fd, buf, 99);
    buf[rv] = 0;
    BIO_printf(bio_out, "Content: '");
    while (rv > 0) {
        BIO_printf(bio_out, "%s", buf);
        rv = app_read(fd, buf, 99);
        buf[rv] = 0;
    }
    BIO_printf(bio_out, "'\n");
    if (rv < 0) {
        TEST_error("Error reading from file '%s': %s\n", test_get_argument(1), strerror(errno));
        return 0;
    }
    if (app_close(fd) < 0) {
        TEST_error("Error closing file '%s': %s\n", test_get_argument(1), strerror(errno));
        return 0;
    }
    return 1;
}

static int test_app_fdopen(void)
{
    int fd;
    char c;
    FILE *file;

    if (test_get_argument_count() != 2) {
        TEST_error("Usage: %s: posix_file_io file_to_read\n", binname);
        return 0;
    }

    fd = app_open(test_get_argument(1), O_RDONLY, 0);
    if (fd < 0) {
        TEST_error("Error opening file '%s': %s\n", test_get_argument(1), strerror(errno));
        return 0;
    }
    file = app_fdopen(fd, "r");
    if (file == NULL) {
        TEST_error("Error opening file '%s': %s\n", test_get_argument(1), strerror(errno));
        return 0;
    }
    BIO_printf(bio_out, "Content: '");
    while (!feof(file)) {
        c = fgetc(file);
        if (!feof(file))
            BIO_printf(bio_out, "%c", c);
    }
    BIO_printf(bio_out, "'\n");
    if (fclose(file) < 0) {
        TEST_error("Error closing file '%s': %s\n", test_get_argument(1), strerror(errno));
        return 0;
    }
    return 1;
}

static int test_int_2_size_t(void)
{
    int in_value;
    size_t out_value;

    if (test_get_argument_count() != 2) {
        TEST_error("Usage: %s: int_2_size_t value.\n", binname);
        return 0;
    }
    if (!str_2_int(test_get_argument(1), &in_value)) {
        TEST_error("Value '%s' is out of range.\n", test_get_argument(1));
        return 0;
    }
    if (!int_2_size_t(in_value, &out_value)) {
        TEST_error("Error converting '%i' to size_t.\n", in_value);
        return 0;
    }
    BIO_printf(bio_out, "Result: '%zu'\n", out_value);

    return 1;
}

static int test_size_t_2_int(void)
{
    size_t in_value;
    int out_value;

    if (test_get_argument_count() != 2) {
        TEST_error("Usage: %s: size_t_2_int value\n", binname);
        return 0;
    }
    if (!str_2_size_t(test_get_argument(1), &in_value)) {
        TEST_error("Value '%s' is out of range.\n", test_get_argument(1));
        return 0;
    }
    if (!size_t_2_int(in_value, &out_value)) {
        TEST_error("Error converting '%zu' to int.\n", in_value);
        return 0;
    }
    BIO_printf(bio_out, "Result: '%i'\n", out_value);

    return 1;
}

static int test_str_2_size_t(void)
{
    size_t out_value;

    if (test_get_argument_count() != 2) {
        TEST_error("Usage: %s: str_2_size_t value\n", binname);
        return 0;
    }
    if (!str_2_size_t(test_get_argument(1), &out_value)) {
        return 0;
    }
    BIO_printf(bio_out, "Result: '%zu'\n", out_value);

    return 1;
}

static int test_str_2_int(void)
{
    int out_value;

    if (test_get_argument_count() != 2) {
        TEST_error("Usage: %s: str_2_int value\n", binname);
        return 0;
    }
    if (!str_2_int(test_get_argument(1), &out_value)) {
        return 0;
    }
    BIO_printf(bio_out, "Result: '%i'\n", out_value);

    return 1;
}

int setup_tests(void)
{
    char *command = test_get_argument(0);

    if (test_get_argument_count()<1) {
        TEST_error("%s: no command specified for testing\n", binname);
        return 0;
    }

    if (strcmp(command, "app_rename") == 0)
        return test_app_rename();
    if (strcmp(command, "app_strcasecmp") == 0)
        return test_app_strcasecmp();
    if (strcmp(command, "posix_file_io") == 0)
        return test_posix_file_io();
    if (strcmp(command, "app_fdopen") == 0)
        return test_app_fdopen();
    if (strcmp(command, "str_2_size_t") == 0)
        return test_str_2_size_t();
    if (strcmp(command, "str_2_int") == 0)
        return test_str_2_int();
    if (strcmp(command, "int_2_size_t") == 0)
        return test_int_2_size_t();
    if (strcmp(command, "size_t_2_int") == 0)
        return test_size_t_2_int();
    
    TEST_error("%s: command '%s' is not supported for testing\n", binname, command);
    return 0;
}

