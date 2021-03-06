/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OSSL_APPS_OS_SPECIFIC_H
# define OSSL_APPS_OS_SPECIFIC_H

/* moved from apps.h */

# include "internal/sockets.h" /* for openssl_fdset() */

extern BIO *bio_in;
extern BIO *bio_out;
extern BIO *bio_err;

BIO *dup_bio_in(int format);
BIO *dup_bio_out(int format);
BIO *dup_bio_err(int format);
BIO *bio_open_owner(const char *filename, int format, int private);
BIO *bio_open_default(const char *filename, char mode, int format);
BIO *bio_open_default_quiet(const char *filename, char mode, int format);
void wait_for_async(SSL *s);
# if defined(OPENSSL_SYS_MSDOS)
int has_stdin_waiting(void);
# endif

void unbuffer(FILE *fp);

void* app_malloc(int sz, const char *what);

int app_isdir(const char *);
int app_access(const char *, int flag);
int fileno_stdin(void);
int fileno_stdout(void);
int raw_read_stdin(void *, int);
int raw_write_stdout(const void *, int);

# define TM_START        0
# define TM_STOP         1
double app_tminterval(int stop, int usertime);

/* moved from apps.c */

#if !defined(_POSIX_C_SOURCE) && defined(OPENSSL_SYS_VMS)
/*
 * On VMS, you need to define this to get the declaration of fileno().  The
 * value 2 is to make sure no function defined in POSIX-2 is left undefined.
 */
# define _POSIX_C_SOURCE 2
#endif

BIO *bio_open_default_(const char *filename, char mode, int format,
                              int quiet);

#endif
