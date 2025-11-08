#pragma once
#include <ngx_core.h>
#include <ngx_http.h>

ngx_int_t bastet_render_page(ngx_http_request_t *r, ngx_str_t challenge_json, ngx_str_t *out);
void bastet_add_security_headers(ngx_http_request_t *r);
ngx_table_elt_t *bastet_find_header(ngx_http_request_t *r, const char *name);
