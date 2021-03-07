/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OSSL_APPS_EXTRACTED_H
# define OSSL_APPS_EXTRACTED_H

# include <openssl/x509.h>
# include "apps_os_specific.h"

//CONF *app_load_config_bio(BIO *in, const char *filename);
//#define app_load_config(filename) app_load_config_internal(filename, 0)
//#define app_load_config_quiet(filename) app_load_config_internal(filename, 1)
//CONF *app_load_config_internal(const char *filename, int quiet);
CONF *app_load_config_verbose(const char *filename, int verbose);
//int app_load_modules(const CONF *config);
//
//int set_cert_times(X509 *x, const char *startdate, const char *enddate,
//                   int days);
//int set_crl_lastupdate(X509_CRL *crl, const char *lastupdate);
//int set_crl_nextupdate(X509_CRL *crl, const char *nextupdate,
//                       long days, long hours, long secs);
//
int set_nameopt(const char *arg);
unsigned long get_nameopt(void);
//int set_cert_ex(unsigned long *flags, const char *arg);
//int set_name_ex(unsigned long *flags, const char *arg);
//int set_ext_copy(int *copy_type, const char *arg);
//int copy_extensions(X509 *x, X509_REQ *req, int copy_type);
char *get_passwd(const char *pass, const char *desc);
int app_passwd(const char *arg1, const char *arg2, char **pass1, char **pass2);
//int add_oid_section(CONF *conf);
//X509_REQ *load_csr(const char *file, int format, const char *desc);
//X509 *load_cert_pass(const char *uri, int maybe_stdin,
//                     const char *pass, const char *desc);
//void cleanse(char *str);
//EVP_PKEY *load_key(const char *uri, int format, int maybe_stdin,
//                   const char *pass, ENGINE *e, const char *desc);
//int load_key_certs_crls(const char *uri, int maybe_stdin,
//                        const char *pass, const char *desc,
//                        EVP_PKEY **ppkey, EVP_PKEY **ppubkey,
//                        EVP_PKEY **pparams,
//                        X509 **pcert, STACK_OF(X509) **pcerts,
//                        X509_CRL **pcrl, STACK_OF(X509_CRL) **pcrls);
//
//void release_engine(ENGINE *e);
//
BIGNUM *load_serial(const char *serialfile, int create, ASN1_INTEGER **retai);
int save_serial(const char *serialfile, const char *suffix, const BIGNUM *serial,
                ASN1_INTEGER **retai);
int rotate_serial(const char *serialfile, const char *new_suffix,
                  const char *old_suffix);
int rand_serial(BIGNUM *b, ASN1_INTEGER *ai);
CA_DB *load_index(const char *dbfile, DB_ATTR *dbattr);
int index_index(CA_DB *db);
int save_index(const char *dbfile, const char *suffix, CA_DB *db);
int rotate_index(const char *dbfile, const char *new_suffix,
                 const char *old_suffix);
void free_index(CA_DB *db);
# define index_name_cmp_noconst(a, b) \
        index_name_cmp((const OPENSSL_CSTRING *)CHECKED_PTR_OF(OPENSSL_STRING, a), \
        (const OPENSSL_CSTRING *)CHECKED_PTR_OF(OPENSSL_STRING, b)) 
int index_name_cmp(const OPENSSL_CSTRING *a, const OPENSSL_CSTRING *b);
int parse_yesno(const char *str, int def);

X509_NAME *parse_name(const char *str, int chtype, int multirdn,
                      const char *desc);

int pkey_ctrl_string(EVP_PKEY_CTX *ctx, const char *value);
//int x509_ctrl_string(X509 *x, const char *value);
//int do_X509_sign(X509 *x, EVP_PKEY *pkey, const EVP_MD *md,
//                 STACK_OF(OPENSSL_STRING) *sigopts, X509V3_CTX *ext_ctx);
//int do_X509_REQ_sign(X509_REQ *x, EVP_PKEY *pkey, const EVP_MD *md,
//                     STACK_OF(OPENSSL_STRING) *sigopts);
//int do_X509_REQ_verify(X509_REQ *x, EVP_PKEY *pkey,
//                       STACK_OF(OPENSSL_STRING) *vfyopts);

//OSSL_LIB_CTX *app_create_libctx(void);
//OSSL_LIB_CTX *app_get0_libctx(void);
//
int app_provider_load(OSSL_LIB_CTX *libctx, const char *provider_name);
void app_providers_cleanup(void);

OSSL_LIB_CTX *app_get0_libctx(void);
int app_set_propq(const char *arg);
const char *app_get0_propq(void);

/* extracted from apps.c */
#define PASS_SOURCE_SIZE_MAX 4

typedef struct {
    const char *name;
    unsigned long flag;
    unsigned long mask;
} NAME_EX_TBL;

/* newly added */

/* removed static */
int do_pkey_ctx_init(EVP_PKEY_CTX *pkctx, STACK_OF(OPENSSL_STRING) *opts);

#endif
