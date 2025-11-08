#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern int bastet_set_secret(const unsigned char *ptr, unsigned long len);
extern int bastet_validate_cookie(const unsigned char *ptr, unsigned long len);
extern int bastet_issue_challenge(const char **json_out, uint16_t difficulty_bits, unsigned long ttl_secs);

typedef enum {
    BASTET_MODE_EXCLUSION = 0,
    BASTET_MODE_INCLUSION,
} bastet_mode_t;

typedef struct {
    ngx_flag_t mode; // enabled: inclusion. disabled: exclusion
    ngx_flag_t allow_x_bastet;
    ngx_str_t  secret;
    uint16_t   difficulty;
    ngx_uint_t ttl;
} ngx_http_bastet_main_conf_t;

typedef struct ngx_http_bastet_loc_conf {
    ngx_flag_t enable;
} ngx_http_bastet_loc_conf_t;

static void *ngx_http_bastet_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_bastet_init_main_conf(ngx_conf_t *cf, void *conf);
static void *ngx_http_bastet_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_bastet_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static char *ngx_http_bastet_toggle(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_bastet_allow_x_bastet(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_bastet_mode(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_bastet_secret(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_bastet_difficulty(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_bastet_ttl(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_bastet_postconfig(ngx_conf_t *cf);
static ngx_int_t ngx_http_bastet_access_handler(ngx_http_request_t *r);