/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef OPENSSL_NO_POSIX_IO
# include <sys/stat.h>
# include <fcntl.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/http.h>
#include <openssl/pem.h>
#include <openssl/store.h>
#include <openssl/pkcs12.h>
#include <openssl/ui.h>
#include <openssl/safestack.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/ssl.h>
#include <openssl/store.h>
#include "s_apps.h"
#include "apps.h"

DEFINE_STACK_OF(CONF)

int app_init(long mesgwin);

int chopup_args(ARGS *arg, char *buf)
{
    int quoted;
    char c = '\0', *p = NULL;

    arg->argc = 0;
    if (arg->size == 0) {
        arg->size = 20;
        arg->argv = app_malloc(sizeof(*arg->argv) * arg->size, "argv space");
    }

    for (p = buf;;) {
        /* Skip whitespace. */
        while (*p && isspace(_UC(*p)))
            p++;
        if (*p == '\0')
            break;

        /* The start of something good :-) */
        if (arg->argc >= arg->size) {
            char **tmp;
            arg->size += 20;
            tmp = OPENSSL_realloc(arg->argv, sizeof(*arg->argv) * arg->size);
            if (tmp == NULL)
                return 0;
            arg->argv = tmp;
        }
        quoted = *p == '\'' || *p == '"';
        if (quoted)
            c = *p++;
        arg->argv[arg->argc++] = p;

        /* now look for the end of this */
        if (quoted) {
            while (*p && *p != c)
                p++;
            *p++ = '\0';
        } else {
            while (*p && !isspace(_UC(*p)))
                p++;
            if (*p)
                *p++ = '\0';
        }
    }
    arg->argv[arg->argc] = NULL;
    return 1;
}

#ifndef APP_INIT
int app_init(long mesgwin)
{
    return 1;
}
#endif

int ctx_set_verify_locations(SSL_CTX *ctx,
                             const char *CAfile, int noCAfile,
                             const char *CApath, int noCApath,
                             const char *CAstore, int noCAstore)
{
    if (CAfile == NULL && CApath == NULL && CAstore == NULL) {
        if (!noCAfile && SSL_CTX_set_default_verify_file(ctx) <= 0)
            return 0;
        if (!noCApath && SSL_CTX_set_default_verify_dir(ctx) <= 0)
            return 0;
        if (!noCAstore && SSL_CTX_set_default_verify_store(ctx) <= 0)
            return 0;

        return 1;
    }

    if (CAfile != NULL && !SSL_CTX_load_verify_file(ctx, CAfile))
        return 0;
    if (CApath != NULL && !SSL_CTX_load_verify_dir(ctx, CApath))
        return 0;
    if (CAstore != NULL && !SSL_CTX_load_verify_store(ctx, CAstore))
        return 0;
    return 1;
}

#ifndef OPENSSL_NO_CT

int ctx_set_ctlog_list_file(SSL_CTX *ctx, const char *path)
{
    if (path == NULL)
        return SSL_CTX_set_default_ctlog_list_file(ctx);

    return SSL_CTX_set_ctlog_list_file(ctx, path);
}

#endif

int dump_cert_text(BIO *out, X509 *x)
{
    print_name(out, "subject=", X509_get_subject_name(x), get_nameopt());
    BIO_puts(out, "\n");
    print_name(out, "issuer=", X509_get_issuer_name(x), get_nameopt());
    BIO_puts(out, "\n");

    return 0;
}

int wrap_password_callback(char *buf, int bufsiz, int verify, void *userdata)
{
    return password_callback(buf, bufsiz, verify, (PW_CB_DATA *)userdata);
}

char *get_passwd(const char *pass, const char *desc)
{
    char *result = NULL;

    if (desc == NULL)
        desc = "<unknown>";
    if (!app_passwd(pass, NULL, &result, NULL))
        BIO_printf(bio_err, "Error getting password for %s\n", desc);
    if (pass != NULL && result == NULL) {
        BIO_printf(bio_err,
                   "Trying plain input string (better precede with 'pass:')\n");
        result = OPENSSL_strdup(pass);
        if (result == NULL)
            BIO_printf(bio_err, "Out of memory getting password for %s\n", desc);
    }
    return result;
}

CONF *app_load_config_modules(const char *configfile)
{
    CONF *conf = NULL;

    if (configfile != NULL) {
        if ((conf = app_load_config_verbose(configfile, 1)) == NULL)
            return NULL;
        if (configfile != default_config_file && !app_load_modules(conf)) {
            NCONF_free(conf);
            conf = NULL;
        }
    }
    return conf;
}

void clear_free(char *str)
{
    if (str != NULL)
        OPENSSL_clear_free(str, strlen(str));
}

EVP_PKEY *load_pubkey(const char *uri, int format, int maybe_stdin,
                      const char *pass, ENGINE *e, const char *desc)
{
    EVP_PKEY *pkey = NULL;
    char *allocated_uri = NULL;

    if (desc == NULL)
        desc = "public key";

    if (format == FORMAT_ENGINE) {
        uri = allocated_uri = make_engine_uri(e, uri, desc);
    }
    (void)load_key_certs_crls(uri, maybe_stdin, pass, desc,
                              NULL, &pkey, NULL, NULL, NULL, NULL, NULL);

    OPENSSL_free(allocated_uri);
    return pkey;
}

EVP_PKEY *load_keyparams(const char *uri, int maybe_stdin, const char *keytype,
                         const char *desc)
{
    EVP_PKEY *params = NULL;

    if (desc == NULL)
        desc = "key parameters";

    (void)load_key_certs_crls(uri, maybe_stdin, NULL, desc,
                              NULL, NULL, &params, NULL, NULL, NULL, NULL);
    if (params != NULL && keytype != NULL && !EVP_PKEY_is_a(params, keytype)) {
        BIO_printf(bio_err,
                   "Unable to load %s from %s (unexpected parameters type)\n",
                   desc, uri);
        ERR_print_errors(bio_err);
        EVP_PKEY_free(params);
        params = NULL;
    }
    return params;
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

void* app_malloc(int sz, const char *what)
{
    void *vp = OPENSSL_malloc(sz);

    if (vp == NULL)
        app_bail_out("%s: Could not allocate %d bytes for %s\n",
                     opt_getprog(), sz, what);
    return vp;
}

/*
 * Initialize or extend, if *certs != NULL, a certificate stack.
 * The caller is responsible for freeing *certs if its value is left not NULL.
 */
int load_certs(const char *uri, STACK_OF(X509) **certs,
               const char *pass, const char *desc)
{
    int was_NULL = *certs == NULL;
    int ret = load_key_certs_crls(uri, 0, pass, desc, NULL, NULL, NULL,
                                  NULL, certs, NULL, NULL);

    if (!ret && was_NULL) {
        sk_X509_pop_free(*certs, X509_free);
        *certs = NULL;
    }
    return ret;
}

/*
 * Initialize or extend, if *crls != NULL, a certificate stack.
 * The caller is responsible for freeing *crls if its value is left not NULL.
 */
int load_crls(const char *uri, STACK_OF(X509_CRL) **crls,
              const char *pass, const char *desc)
{
    int was_NULL = *crls == NULL;
    int ret = load_key_certs_crls(uri, 0, pass, desc, NULL, NULL, NULL,
                                  NULL, NULL, NULL, crls);

    if (!ret && was_NULL) {
        sk_X509_CRL_pop_free(*crls, X509_CRL_free);
        *crls = NULL;
    }
    return ret;
}

void print_name(BIO *out, const char *title, const X509_NAME *nm,
                unsigned long lflags)
{
    char *buf;
    char mline = 0;
    int indent = 0;

    if (title)
        BIO_puts(out, title);
    if ((lflags & XN_FLAG_SEP_MASK) == XN_FLAG_SEP_MULTILINE) {
        mline = 1;
        indent = 4;
    }
    if (lflags == XN_FLAG_COMPAT) {
        buf = X509_NAME_oneline(nm, 0, 0);
        BIO_puts(out, buf);
        BIO_puts(out, "\n");
        OPENSSL_free(buf);
    } else {
        if (mline)
            BIO_puts(out, "\n");
        X509_NAME_print_ex(out, nm, indent, lflags);
        BIO_puts(out, "\n");
    }
}

void print_bignum_var(BIO *out, const BIGNUM *in, const char *var,
                      int len, unsigned char *buffer)
{
    BIO_printf(out, "    static unsigned char %s_%d[] = {", var, len);
    if (BN_is_zero(in)) {
        BIO_printf(out, "\n        0x00");
    } else {
        int i, l;

        l = BN_bn2bin(in, buffer);
        for (i = 0; i < l; i++) {
            BIO_printf(out, (i % 10) == 0 ? "\n        " : " ");
            if (i < l - 1)
                BIO_printf(out, "0x%02X,", buffer[i]);
            else
                BIO_printf(out, "0x%02X", buffer[i]);
        }
    }
    BIO_printf(out, "\n    };\n");
}

void print_array(BIO *out, const char* title, int len, const unsigned char* d)
{
    int i;

    BIO_printf(out, "unsigned char %s[%d] = {", title, len);
    for (i = 0; i < len; i++) {
        if ((i % 10) == 0)
            BIO_printf(out, "\n    ");
        if (i < len - 1)
            BIO_printf(out, "0x%02X, ", d[i]);
        else
            BIO_printf(out, "0x%02X", d[i]);
    }
    BIO_printf(out, "\n};\n");
}

X509_STORE *setup_verify(const char *CAfile, int noCAfile,
                         const char *CApath, int noCApath,
                         const char *CAstore, int noCAstore)
{
    X509_STORE *store = X509_STORE_new();
    X509_LOOKUP *lookup;
    OSSL_LIB_CTX *libctx = app_get0_libctx();
    const char *propq = app_get0_propq();

    if (store == NULL)
        goto end;

    if (CAfile != NULL || !noCAfile) {
        lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());
        if (lookup == NULL)
            goto end;
        if (CAfile != NULL) {
            if (!X509_LOOKUP_load_file_ex(lookup, CAfile, X509_FILETYPE_PEM,
                                          libctx, propq)) {
                BIO_printf(bio_err, "Error loading file %s\n", CAfile);
                goto end;
            }
        } else {
            X509_LOOKUP_load_file_ex(lookup, NULL, X509_FILETYPE_DEFAULT,
                                     libctx, propq);
        }
    }

    if (CApath != NULL || !noCApath) {
        lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
        if (lookup == NULL)
            goto end;
        if (CApath != NULL) {
            if (!X509_LOOKUP_add_dir(lookup, CApath, X509_FILETYPE_PEM)) {
                BIO_printf(bio_err, "Error loading directory %s\n", CApath);
                goto end;
            }
        } else {
            X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);
        }
    }

    if (CAstore != NULL || !noCAstore) {
        lookup = X509_STORE_add_lookup(store, X509_LOOKUP_store());
        if (lookup == NULL)
            goto end;
        if (!X509_LOOKUP_add_store_ex(lookup, CAstore, libctx, propq)) {
            if (CAstore != NULL)
                BIO_printf(bio_err, "Error loading store URI %s\n", CAstore);
            goto end;
        }
    }

    ERR_clear_error();
    return store;
 end:
    ERR_print_errors(bio_err);
    X509_STORE_free(store);
    return NULL;
}

/*
 * Read whole contents of a BIO into an allocated memory buffer and return
 * it.
 */

int bio_to_mem(unsigned char **out, int maxlen, BIO *in)
{
    BIO *mem;
    int len, ret;
    unsigned char tbuf[1024];

    mem = BIO_new(BIO_s_mem());
    if (mem == NULL)
        return -1;
    for (;;) {
        if ((maxlen != -1) && maxlen < 1024)
            len = maxlen;
        else
            len = 1024;
        len = BIO_read(in, tbuf, len);
        if (len < 0) {
            BIO_free(mem);
            return -1;
        }
        if (len == 0)
            break;
        if (BIO_write(mem, tbuf, len) != len) {
            BIO_free(mem);
            return -1;
        }
        maxlen -= len;

        if (maxlen == 0)
            break;
    }
    ret = BIO_get_mem_data(mem, (char **)out);
    BIO_set_flags(mem, BIO_FLAGS_MEM_RDONLY);
    BIO_free(mem);
    return ret;
}

static void nodes_print(const char *name, STACK_OF(X509_POLICY_NODE) *nodes)
{
    X509_POLICY_NODE *node;
    int i;

    BIO_printf(bio_err, "%s Policies:", name);
    if (nodes) {
        BIO_puts(bio_err, "\n");
        for (i = 0; i < sk_X509_POLICY_NODE_num(nodes); i++) {
            node = sk_X509_POLICY_NODE_value(nodes, i);
            X509_POLICY_NODE_print(bio_err, node, 2);
        }
    } else {
        BIO_puts(bio_err, " <empty>\n");
    }
}

void policies_print(X509_STORE_CTX *ctx)
{
    X509_POLICY_TREE *tree;
    int explicit_policy;
    tree = X509_STORE_CTX_get0_policy_tree(ctx);
    explicit_policy = X509_STORE_CTX_get_explicit_policy(ctx);

    BIO_printf(bio_err, "Require explicit Policy: %s\n",
               explicit_policy ? "True" : "False");

    nodes_print("Authority", X509_policy_tree_get0_policies(tree));
    nodes_print("User", X509_policy_tree_get0_user_policies(tree));
}

/*-
 * next_protos_parse parses a comma separated list of strings into a string
 * in a format suitable for passing to SSL_CTX_set_next_protos_advertised.
 *   outlen: (output) set to the length of the resulting buffer on success.
 *   err: (maybe NULL) on failure, an error message line is written to this BIO.
 *   in: a NUL terminated string like "abc,def,ghi"
 *
 *   returns: a malloc'd buffer or NULL on failure.
 */
unsigned char *next_protos_parse(size_t *outlen, const char *in)
{
    size_t len;
    unsigned char *out;
    size_t i, start = 0;
    size_t skipped = 0;

    len = strlen(in);
    if (len == 0 || len >= 65535)
        return NULL;

    out = app_malloc(len + 1, "NPN buffer");
    for (i = 0; i <= len; ++i) {
        if (i == len || in[i] == ',') {
            /*
             * Zero-length ALPN elements are invalid on the wire, we could be
             * strict and reject the entire string, but just ignoring extra
             * commas seems harmless and more friendly.
             *
             * Every comma we skip in this way puts the input buffer another
             * byte ahead of the output buffer, so all stores into the output
             * buffer need to be decremented by the number commas skipped.
             */
            if (i == start) {
                ++start;
                ++skipped;
                continue;
            }
            if (i - start > 255) {
                OPENSSL_free(out);
                return NULL;
            }
            out[start-skipped] = (unsigned char)(i - start);
            start = i + 1;
        } else {
            out[i + 1 - skipped] = in[i];
        }
    }

    if (len <= skipped) {
        OPENSSL_free(out);
        return NULL;
    }

    *outlen = len + 1 - skipped;
    return out;
}

void print_cert_checks(BIO *bio, X509 *x,
                       const char *checkhost,
                       const char *checkemail, const char *checkip)
{
    if (x == NULL)
        return;
    if (checkhost) {
        BIO_printf(bio, "Hostname %s does%s match certificate\n",
                   checkhost,
                   X509_check_host(x, checkhost, 0, 0, NULL) == 1
                       ? "" : " NOT");
    }

    if (checkemail) {
        BIO_printf(bio, "Email %s does%s match certificate\n",
                   checkemail, X509_check_email(x, checkemail, 0, 0)
                   ? "" : " NOT");
    }

    if (checkip) {
        BIO_printf(bio, "IP %s does%s match certificate\n",
                   checkip, X509_check_ip_asc(x, checkip, 0) ? "" : " NOT");
    }
}

#ifndef OPENSSL_NO_SOCK
static const char *tls_error_hint(void)
{
    unsigned long err = ERR_peek_error();

    if (ERR_GET_LIB(err) != ERR_LIB_SSL)
        err = ERR_peek_last_error();
    if (ERR_GET_LIB(err) != ERR_LIB_SSL)
        return NULL;

    switch (ERR_GET_REASON(err)) {
    case SSL_R_WRONG_VERSION_NUMBER:
        return "The server does not support (a suitable version of) TLS";
    case SSL_R_UNKNOWN_PROTOCOL:
        return "The server does not support HTTPS";
    case SSL_R_CERTIFICATE_VERIFY_FAILED:
        return "Cannot authenticate server via its TLS certificate, likely due to mismatch with our trusted TLS certs or missing revocation status";
    case SSL_AD_REASON_OFFSET + TLS1_AD_UNKNOWN_CA:
        return "Server did not accept our TLS certificate, likely due to mismatch with server's trust anchor or missing revocation status";
    case SSL_AD_REASON_OFFSET + SSL3_AD_HANDSHAKE_FAILURE:
        return "TLS handshake failure. Possibly the server requires our TLS certificate but did not receive it";
    default: /* no error or no hint available for error */
        return NULL;
    }
}

/* HTTP callback function that supports TLS connection also via HTTPS proxy */
BIO *app_http_tls_cb(BIO *hbio, void *arg, int connect, int detail)
{
    APP_HTTP_TLS_INFO *info = (APP_HTTP_TLS_INFO *)arg;
    SSL_CTX *ssl_ctx = info->ssl_ctx;
    SSL *ssl;
    BIO *sbio = NULL;

    if (connect && detail) { /* connecting with TLS */
        if ((info->use_proxy
             && !OSSL_HTTP_proxy_connect(hbio, info->server, info->port,
                                         NULL, NULL, /* no proxy credentials */
                                         info->timeout, bio_err, opt_getprog()))
                || (sbio = BIO_new(BIO_f_ssl())) == NULL) {
            return NULL;
        }
        if (ssl_ctx == NULL || (ssl = SSL_new(ssl_ctx)) == NULL) {
            BIO_free(sbio);
            return NULL;
        }

        SSL_set_tlsext_host_name(ssl, info->server);

        SSL_set_connect_state(ssl);
        BIO_set_ssl(sbio, ssl, BIO_CLOSE);

        hbio = BIO_push(sbio, hbio);
    } else if (!connect && !detail) { /* disconnecting after error */
        const char *hint = tls_error_hint();
        if (hint != NULL)
            ERR_add_error_data(2, " : ", hint);
        /*
         * If we pop sbio and BIO_free() it this may lead to libssl double free.
         * Rely on BIO_free_all() done by OSSL_HTTP_transfer() in http_client.c
         */
    }
    return hbio;
}

ASN1_VALUE *app_http_get_asn1(const char *url, const char *proxy,
                              const char *no_proxy, SSL_CTX *ssl_ctx,
                              const STACK_OF(CONF_VALUE) *headers,
                              long timeout, const char *expected_content_type,
                              const ASN1_ITEM *it)
{
    APP_HTTP_TLS_INFO info;
    char *server;
    char *port;
    int use_ssl;
    ASN1_VALUE *resp = NULL;

    if (url == NULL || it == NULL) {
        ERR_raise(ERR_LIB_HTTP, ERR_R_PASSED_NULL_PARAMETER);
        return NULL;
    }

    if (!OSSL_HTTP_parse_url(url, &server, &port, NULL, NULL, &use_ssl))
        return NULL;
    if (use_ssl && ssl_ctx == NULL) {
        ERR_raise_data(ERR_LIB_HTTP, ERR_R_PASSED_NULL_PARAMETER,
                       "missing SSL_CTX");
        goto end;
    }

    info.server = server;
    info.port = port;
    info.use_proxy = proxy != NULL;
    info.timeout = timeout;
    info.ssl_ctx = ssl_ctx;
    resp = OSSL_HTTP_get_asn1(url, proxy, no_proxy,
                              NULL, NULL, app_http_tls_cb, &info,
                              headers, 0 /* maxline */, 0 /* max_resp_len */,
                              timeout, expected_content_type, it);
 end:
    OPENSSL_free(server);
    OPENSSL_free(port);
    return resp;

}

ASN1_VALUE *app_http_post_asn1(const char *host, const char *port,
                               const char *path, const char *proxy,
                               const char *no_proxy, SSL_CTX *ssl_ctx,
                               const STACK_OF(CONF_VALUE) *headers,
                               const char *content_type,
                               ASN1_VALUE *req, const ASN1_ITEM *req_it,
                               long timeout, const ASN1_ITEM *rsp_it)
{
    APP_HTTP_TLS_INFO info;

    info.server = host;
    info.port = port;
    info.use_proxy = proxy != NULL;
    info.timeout = timeout;
    info.ssl_ctx = ssl_ctx;
    return OSSL_HTTP_post_asn1(host, port, path, ssl_ctx != NULL,
                               proxy, no_proxy,
                               NULL, NULL, app_http_tls_cb, &info,
                               headers, content_type, req, req_it,
                               0 /* maxline */,
                               0 /* max_resp_len */, timeout, NULL, rsp_it);
}

#endif

/* Corrupt a signature by modifying final byte */
void corrupt_signature(const ASN1_STRING *signature)
{
        unsigned char *s = signature->data;
        s[signature->length - 1] ^= 0x1;
}

int opt_printf_stderr(const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = BIO_vprintf(bio_err, fmt, ap);
    va_end(ap);
    return ret;
}

OSSL_PARAM *app_params_new_from_opts(STACK_OF(OPENSSL_STRING) *opts,
                                     const OSSL_PARAM *paramdefs)
{
    OSSL_PARAM *params = NULL;
    size_t sz = (size_t)sk_OPENSSL_STRING_num(opts);
    size_t params_n;
    char *opt = "", *stmp, *vtmp = NULL;
    int found = 1;

    if (opts == NULL)
        return NULL;

    params = OPENSSL_zalloc(sizeof(OSSL_PARAM) * (sz + 1));
    if (params == NULL)
        return NULL;

    for (params_n = 0; params_n < sz; params_n++) {
        opt = sk_OPENSSL_STRING_value(opts, (int)params_n);
        if ((stmp = OPENSSL_strdup(opt)) == NULL
            || (vtmp = strchr(stmp, ':')) == NULL)
            goto err;
        /* Replace ':' with 0 to terminate the string pointed to by stmp */
        *vtmp = 0;
        /* Skip over the separator so that vmtp points to the value */
        vtmp++;
        if (!OSSL_PARAM_allocate_from_text(&params[params_n], paramdefs,
                                           stmp, vtmp, strlen(vtmp), &found))
            goto err;
        OPENSSL_free(stmp);
    }
    params[params_n] = OSSL_PARAM_construct_end();
    return params;
err:
    OPENSSL_free(stmp);
    BIO_printf(bio_err, "Parameter %s '%s'\n", found ? "error" : "unknown",
               opt);
    ERR_print_errors(bio_err);
    app_params_free(params);
    return NULL;
}

void app_params_free(OSSL_PARAM *params)
{
    int i;

    if (params != NULL) {
        for (i = 0; params[i].key != NULL; ++i)
            OPENSSL_free(params[i].data);
        OPENSSL_free(params);
    }
}
