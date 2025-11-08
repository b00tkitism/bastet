#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bastet_assets.h"
#include "ngx_http_bastet_module.h"

static ngx_command_t ngx_http_bastet_commands[] = {
    {
        ngx_string("bastet_toggle"),
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        ngx_http_bastet_toggle,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_bastet_loc_conf_t, enable),
        NULL
    },

    {
        ngx_string("bastet_allow_x_bastet"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_NOARGS,
        ngx_http_bastet_allow_x_bastet,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },

    {
        ngx_string("bastet_mode"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_http_bastet_mode,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },

    {
        ngx_string("bastet_secret"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_http_bastet_secret,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0, 
        NULL 
    },

    {
        ngx_string("bastet_difficulty_bits"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_http_bastet_difficulty,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },

    { 
        ngx_string("bastet_ttl"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_http_bastet_ttl,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },

    ngx_null_command
};

static ngx_http_module_t ngx_http_bastet_module_ctx = {
    NULL,
    ngx_http_bastet_postconfig,

    ngx_http_bastet_create_main_conf,
    ngx_http_bastet_init_main_conf,

    NULL, NULL,

    ngx_http_bastet_create_loc_conf,
    ngx_http_bastet_merge_loc_conf
};

ngx_module_t ngx_http_bastet_module = {
    NGX_MODULE_V1,
    &ngx_http_bastet_module_ctx,
    ngx_http_bastet_commands,
    NGX_HTTP_MODULE,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NGX_MODULE_V1_PADDING
};

static void *
ngx_http_bastet_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_bastet_main_conf_t *mcf = ngx_pcalloc(cf->pool, sizeof(*mcf));
    if (mcf == NULL)
        return NULL;

    mcf->secret.len = 0; mcf->secret.data = NULL;
    mcf->difficulty = (uint16_t)18;
    mcf->ttl = 120;
    return mcf;
}

static char *
ngx_http_bastet_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_bastet_main_conf_t *mcf = conf;
    if (mcf->secret.len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "bastet: bastet_secret is required");
        return NGX_CONF_ERROR;
    }

    if (bastet_set_secret(mcf->secret.data, (unsigned long)mcf->secret.len) != 1) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "bastet: bastet_set_secret failed");
        return NGX_CONF_ERROR;
    }
    return NGX_CONF_OK;
}

static void *
ngx_http_bastet_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_bastet_loc_conf_t *lc = ngx_pcalloc(cf->pool, sizeof(*lc));
    if (lc == NULL)
        return NULL;

    lc->enable = NGX_CONF_UNSET;
    return lc;
}

static char *
ngx_http_bastet_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_bastet_loc_conf_t *prev = parent;
    ngx_http_bastet_loc_conf_t *conf = child;
    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    return NGX_CONF_OK;
}

static char *
ngx_http_bastet_toggle(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_bastet_loc_conf_t* mcf = conf;
    mcf->enable = 1;
    return NGX_CONF_OK;
}

static char *
ngx_http_bastet_allow_x_bastet(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_bastet_main_conf_t* mcf = conf;
    mcf->allow_x_bastet = 1;
    return NGX_CONF_OK;
}

static char *
ngx_http_bastet_mode(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_bastet_main_conf_t *mcf = conf;
    ngx_str_t *v = cf->args->elts;
    mcf->mode = strncmp((char*)v[1].data, "inclusion", 9) == 0 ? BASTET_MODE_INCLUSION : BASTET_MODE_EXCLUSION;
    return NGX_CONF_OK;
}

static char *
ngx_http_bastet_secret(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_bastet_main_conf_t *mcf = conf;
    ngx_str_t *v = cf->args->elts;
    mcf->secret = v[1];
    return NGX_CONF_OK;
}

static char *
ngx_http_bastet_difficulty(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_bastet_main_conf_t *mcf = conf;
    ngx_str_t *v = cf->args->elts;

    long n = ngx_atoi(v[1].data, v[1].len);
    if (n == NGX_ERROR || n < 0 || n > 256)
        return "invalid bastet_difficulty_bits (0..256)";

    mcf->difficulty = (uint16_t)n;
    return NGX_CONF_OK;
}

static char *
ngx_http_bastet_ttl(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_bastet_main_conf_t *mcf = conf;
    ngx_str_t *v = cf->args->elts;

    long n = ngx_parse_time(&v[1], 0);
    if (n == NGX_ERROR || n <= 0)
        return "invalid bastet_ttl";

    mcf->ttl = (ngx_uint_t)n;
    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_bastet_postconfig(ngx_conf_t *cf)
{
    ngx_http_core_main_conf_t *cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    if (cmcf == NULL) 
        return NGX_ERROR;

    ngx_http_handler_pt *h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL)
        return NGX_ERROR;
    
    *h = ngx_http_bastet_access_handler;

    return NGX_OK;
}

static ngx_int_t
send_inline_html(ngx_http_request_t *r, ngx_str_t body)
{
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = body.len;
    ngx_str_set(&r->headers_out.content_type, "text/html");

    ngx_int_t rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) 
        return rc;

    ngx_buf_t *b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (!b) 
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    b->pos = body.data; b->last = body.data + body.len; b->memory = 1; b->last_buf = 1;
    ngx_chain_t out = { b, NULL };

    return ngx_http_output_filter(r, &out);
}

static ngx_int_t
ngx_http_bastet_access_handler(ngx_http_request_t *r)
{
    ngx_http_bastet_main_conf_t *mc =
        ngx_http_get_module_main_conf(r, ngx_http_bastet_module);

    ngx_http_bastet_loc_conf_t *lc =
        ngx_http_get_module_loc_conf(r, ngx_http_bastet_module);
    
    if (lc == NULL || mc == NULL)
        return NGX_DECLINED;

    if (mc->mode == BASTET_MODE_EXCLUSION && lc->enable == 0)
        return NGX_DECLINED;
    
    if (mc->mode == BASTET_MODE_INCLUSION && lc->enable == 1)
        return NGX_DECLINED;

    if (r->method != NGX_HTTP_GET && r->method != NGX_HTTP_HEAD)
        return NGX_DECLINED;

    ngx_http_bastet_main_conf_t *mcf = ngx_http_get_module_main_conf(r, ngx_http_bastet_module);

    ngx_str_t cookie_name = ngx_string("bastet");
    ngx_str_t value = ngx_string("");

    ngx_table_elt_t *ck = ngx_http_parse_multi_header_lines(r, r->headers_in.cookie, &cookie_name, &value);
    if (!ck && mcf->allow_x_bastet) {
        ngx_list_part_t *part = &r->headers_in.headers.part;
        ngx_table_elt_t *h = part->elts;

        for (ngx_uint_t i = 0;; i++) {
            if (i >= part->nelts) {
                if (part->next == NULL)
                    break;

                part = part->next;
                h = part->elts;
                i = 0;
            }

            if (h[i].key.len == sizeof("X-Bastet") - 1 && 
                ngx_strncasecmp(h[i].key.data, (u_char *)"X-Bastet", sizeof("X-Bastet") - 1) == 0)
            {
                value = h[i].value;
                break;
            }
        }
    }

    if (value.len > 0) {
        if (bastet_validate_cookie(value.data, (unsigned long)value.len) == 1) {
            return NGX_DECLINED;
        }
    }

    const char *challenge_json = NULL;
    if (bastet_issue_challenge(&challenge_json, mcf->difficulty, (unsigned long)mcf->ttl) != 1 || !challenge_json) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_str_t cj;
    cj.len  = ngx_strlen(challenge_json);
    cj.data = ngx_pnalloc(r->pool, cj.len);
    if (!cj.data) { 
        free((void*)challenge_json); 
        return NGX_HTTP_INTERNAL_SERVER_ERROR; 
    }
    
    ngx_memcpy(cj.data, (u_char*)challenge_json, cj.len);
    free((void*)challenge_json);

    ngx_str_t body;
    if (bastet_render_page(r, cj, &body) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    bastet_add_security_headers(r);

    return send_inline_html(r, body);
}
