#include "pow_assets.h"
#include <string.h>

#include "generated/pow_page.inc"
#include "generated/pow_solver.inc"

static const char TOK_CHALLENGE[] = "__CHALLENGE_JSON__";
static const char TOK_SOLVER[]    = "__SOLVER_JS__";

static u_char *memcat(u_char *dst, const u_char *src, size_t len) {
    if (len) ngx_memcpy(dst, src, len);
    return dst + len;
}

static ngx_int_t replace_once(ngx_http_request_t *r,
                              const u_char *buf, size_t len,
                              const char *token,
                              const u_char *rep, size_t rep_len,
                              ngx_str_t *out)
{
    size_t tok_len = ngx_strlen(token);
    if (tok_len == 0) return NGX_ERROR;

    u_char *pos = ngx_strnstr((u_char *)buf, (char *)token, len);
    if (pos == NULL) {
        out->len = len;
        out->data = ngx_pnalloc(r->pool, len);
        if (!out->data) return NGX_ERROR;
        ngx_memcpy(out->data, buf, len);
        return NGX_OK;
    }

    size_t off = (size_t)(pos - buf);
    size_t new_len = off + rep_len + (len - off - tok_len);

    out->data = ngx_pnalloc(r->pool, new_len);
    if (!out->data) return NGX_ERROR;
    out->len = new_len;

    u_char *p = out->data;
    p = memcat(p, buf, off);
    p = memcat(p, rep, rep_len);
    p = memcat(p, pos + tok_len, len - off - tok_len);

    return NGX_OK;
}

ngx_int_t pow_render_page(ngx_http_request_t *r, ngx_str_t challenge_json, ngx_str_t *out)
{
    ngx_str_t step1;
    if (replace_once(r,
                     (const u_char *)POW_PAGE, (size_t)POW_PAGE_len,
                     TOK_CHALLENGE,
                     challenge_json.data, challenge_json.len,
                     &step1) != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (replace_once(r,
                     step1.data, step1.len,
                     TOK_SOLVER,
                     (const u_char *)POW_SOLVER, (size_t)POW_SOLVER_len,
                     out) != NGX_OK)
    {
        return NGX_ERROR;
    }

    return NGX_OK;
}

void pow_add_security_headers(ngx_http_request_t *r)
{
    ngx_table_elt_t *h;

    h = ngx_list_push(&r->headers_out.headers);
    if (h){ h->hash=1; ngx_str_set(&h->key,"X-Frame-Options"); ngx_str_set(&h->value,"DENY"); }

    h = ngx_list_push(&r->headers_out.headers);
    if (h){
        h->hash=1; ngx_str_set(&h->key,"Content-Security-Policy");
        ngx_str_set(&h->value,"frame-ancestors 'none'; object-src 'none'; base-uri 'none'");
    }

    h = ngx_list_push(&r->headers_out.headers);
    if (h){ h->hash=1; ngx_str_set(&h->key,"Cache-Control"); ngx_str_set(&h->value,"no-store, max-age=0, must-revalidate"); }

    h = ngx_list_push(&r->headers_out.headers);
    if (h){ h->hash=1; ngx_str_set(&h->key,"Referrer-Policy"); ngx_str_set(&h->value,"no-referrer"); }
}

ngx_table_elt_t *pow_find_header(ngx_http_request_t *r, const char *name)
{
    ngx_list_part_t *part = &r->headers_in.headers.part;
    ngx_table_elt_t *h = part->elts;
    size_t nlen = ngx_strlen(name);
    for (unsigned i=0;;i++){
        if (i >= part->nelts){
            if (part->next == NULL) break;
            part = part->next; h = part->elts; i = 0;
        }
        if (h[i].key.len == nlen &&
            ngx_strncasecmp(h[i].key.data,(u_char*)name,nlen)==0) return &h[i];
    }
    return NULL;
}
