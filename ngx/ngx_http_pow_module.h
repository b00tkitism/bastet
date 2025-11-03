#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern int pow_set_secret(const unsigned char *ptr, unsigned long len);
extern int pow_validate_cookie(const unsigned char *ptr, unsigned long len);
extern int pow_issue_challenge(const char **json_out, uint16_t difficulty_bits, unsigned long ttl_secs);

typedef struct {
    ngx_str_t  secret;
    uint16_t   difficulty;
    ngx_uint_t ttl;
} ngx_http_pow_main_conf_t;

typedef struct {
    ngx_flag_t enable;
} ngx_http_pow_loc_conf_t;

static void *ngx_http_pow_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_pow_init_main_conf(ngx_conf_t *cf, void *conf);
static void *ngx_http_pow_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_pow_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static char *ngx_http_pow(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_pow_secret(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_pow_difficulty(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_pow_ttl(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_pow_postconfig(ngx_conf_t *cf);
static ngx_int_t ngx_http_pow_access_handler(ngx_http_request_t *r);