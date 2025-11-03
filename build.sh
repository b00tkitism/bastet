set -Eeuo pipefail

### pretty logging
if command -v tput >/dev/null 2>&1; then
  BOLD="$(tput bold || true)"; DIM="$(tput dim || true)"
  RED="$(tput setaf 1 || true)"; GREEN="$(tput setaf 2 || true)"; YEL="$(tput setaf 3 || true)"; BLU="$(tput setaf 4 || true)"
  RESET="$(tput sgr0 || true)"
else
  BOLD=""; DIM=""; RED=""; GREEN=""; YEL=""; BLU=""; RESET=""
fi

log()   { echo -e "${DIM}[$(date +%H:%M:%S)]${RESET} $*"; }
info()  { echo -e "${BLU}${BOLD}▶${RESET} $*"; }
ok()    { echo -e "${GREEN}${BOLD}✔${RESET} $*"; }
warn()  { echo -e "${YEL}${BOLD}⚠${RESET} $*"; }
err()   { echo -e "${RED}${BOLD}✖${RESET} $*"; }
die()   { err "$*"; exit 1; }

### traps
cleanup() { :; }
trap cleanup EXIT
trap 'err "failed at line $LINENO"; exit 1' ERR

### defaults
YES=${YES:-0}
CI_MODE=0
RUST_PROFILE=release
NGINX_SRC_DEFAULT="${NGINX_SOURCE_DIR:-$(pwd)/nginx}"
ADDON_DIR_DEFAULT="${ADDON_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/ngx}"
RUST_DIR_DEFAULT="${RUST_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/powlib}"
INSTALL_LIB_DEFAULT="${INSTALL_LIB:-/usr/local/lib}"
DO_INSTALL_LIB=1
WITH_COMPAT=0
RECONFIGURE=0
CLEAN=0
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

### parse args (simple)
show_help() { sed -n '1,55p' "$0"; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    -y|--yes) YES=1; shift ;;
    --ci) CI_MODE=1; YES=1; DO_INSTALL_LIB=0; shift ;;
    --debug) RUST_PROFILE=debug; shift ;;
    --nginx-source) NGINX_SRC_DEFAULT="$2"; shift 2 ;;
    --addon-dir) ADDON_DIR_DEFAULT="$2"; shift 2 ;;
    --rust-dir) RUST_DIR_DEFAULT="$2"; shift 2 ;;
    --install-lib) INSTALL_LIB_DEFAULT="$2"; shift 2 ;;
    --no-install-lib) DO_INSTALL_LIB=0; shift ;;
    --reconfigure) RECONFIGURE=1; shift ;;
    --jobs) JOBS="$2"; shift 2 ;;
    --clean) CLEAN=1; shift ;;
    -h|--help) show_help; exit 0 ;;
    *) die "unknown option: $1 (use --help)";;
  esac
done

NGINX_SRC="${NGINX_SRC_DEFAULT}"
ADDON_DIR="${ADDON_DIR_DEFAULT}"
RUST_DIR="${RUST_DIR_DEFAULT}"
GEN_DIR="$ADDON_DIR/generated"
ASSETS_DIR="$ADDON_DIR/assets"
INSTALL_LIB="${INSTALL_LIB_DEFAULT}"

need() { command -v "$1" >/dev/null 2>&1 || die "missing dependency: $1"; }
ask_yes_no() {
  local prompt="${1:-Continue?}"; local default="${2:-Y}"
  if [[ "$YES" == "1" ]]; then return 0; fi
  local suffix="[y/N]"; [[ "$default" =~ ^[Yy]$ ]] && suffix="[Y/n]"
  read -r -p "$(echo -e "${BOLD}${prompt}${RESET} ${suffix} ")" ans || true
  ans="${ans:-$default}"; [[ "$ans" =~ ^[Yy]$ ]] && return 0 || return 1
}

echo
info "PoW NGINX module – interactive builder"
log  "NGINX source : ${NGINX_SRC}"
log  "Addon (C)    : ${ADDON_DIR}"
log  "Rust cdylib  : ${RUST_DIR}"
log  "Install lib  : ${INSTALL_LIB} (install: ${DO_INSTALL_LIB})"
log  "Rust profile : ${RUST_PROFILE}"
log  "Jobs         : ${JOBS}"
echo

need xxd
need cargo
need make
need gcc
[[ -d "$NGINX_SRC" ]] || die "NGINX source not found at: $NGINX_SRC"
[[ -x "$NGINX_SRC/auto/configure" ]] || die "This does not look like an NGINX source tree: $NGINX_SRC"
[[ -d "$ADDON_DIR" ]] || die "addon dir not found: $ADDON_DIR"
[[ -d "$RUST_DIR" ]] || die "rust dir not found: $RUST_DIR"
[[ -f "$ASSETS_DIR/page.html" && -f "$ASSETS_DIR/solver.js" ]] || die "missing assets in $ASSETS_DIR (need page.html, solver.js)"

if [[ "$DO_INSTALL_LIB" == "1" ]]; then
  need ldconfig
  if [[ $EUID -ne 0 ]]; then
    if ! sudo -v >/dev/null 2>&1; then
      die "sudo required to install into $INSTALL_LIB; run with --no-install-lib or ensure sudo works."
    fi
  fi
fi

if [[ "$CLEAN" == "1" ]]; then
  info "Cleaning generated assets at ${GEN_DIR}"
  rm -rf "$GEN_DIR"
fi

if ! ask_yes_no "Proceed with build steps?"; then
  warn "aborted."
  exit 0
fi

info "[1/5] Generating embedded assets with xxd -i"
mkdir -p "$GEN_DIR"
xxd -i -n POW_PAGE   "$ASSETS_DIR/page.html"   > "$GEN_DIR/pow_page.inc"
xxd -i -n POW_SOLVER "$ASSETS_DIR/solver.js"   > "$GEN_DIR/pow_solver.inc"
ok "assets generated → ${GEN_DIR}"

info "[2/5] Building Rust cdylib (profile: ${RUST_PROFILE})"
pushd "$RUST_DIR" >/dev/null
if [[ "$RUST_PROFILE" == "release" ]]; then
  cargo build --release
  LIB_PATH="$RUST_DIR/target/release/libpow.so"
else
  cargo build
  LIB_PATH="$RUST_DIR/target/debug/libpow.so"
fi
popd >/dev/null
[[ -f "$LIB_PATH" ]] || die "expected Rust output at $LIB_PATH"
ok "built ${LIB_PATH}"

if [[ "$DO_INSTALL_LIB" == "1" ]]; then
  info "[3/5] Installing libpow.so → ${INSTALL_LIB}"
  sudo mkdir -p "$INSTALL_LIB"
  sudo cp -f "$LIB_PATH" "$INSTALL_LIB/libpow.so"
  sudo chmod 755 "$INSTALL_LIB/libpow.so"
  sudo ldconfig || true
  ok "installed ${INSTALL_LIB}/libpow.so"
else
  warn "[3/5] Skipping system install (use --no-install-lib)."
fi

info "[4/5] Configuring NGINX with dynamic module"
pushd "$NGINX_SRC" >/dev/null

CONFIGURE_ARGS=( "--add-dynamic-module=${ADDON_DIR}" )

./auto/configure "${CONFIGURE_ARGS[@]}"
popd >/dev/null
ok "configured NGINX with addon"

info "[5/5] Building NGINX dynamic modules"
pushd "$NGINX_SRC" >/dev/null
make -j"${JOBS}" modules
popd >/dev/null

MOD_SO="$NGINX_SRC/objs/ngx_http_pow_module.so"
[[ -f "$MOD_SO" ]] || die "module .so not found at ${MOD_SO}"
ok "built module → ${MOD_SO}"

echo
ok "Done."
echo -e "${BOLD}Artifacts:${RESET}"
echo "  - Dynamic module: ${MOD_SO}"
if [[ "$DO_INSTALL_LIB" == "1" ]]; then
  echo "  - Rust library  : ${INSTALL_LIB}/libpow.so"
else
  echo "  - Rust library  : ${LIB_PATH}"
fi
echo

clear
cat <<'EOF'

Next steps:

1) Load the dynamic module in your nginx.conf (top-level):
   load_module modules/ngx_http_pow_module.so;

2) Use the directive in a server/location (example):
   http {
     server {
       listen 8080;

       # enable PoW check on this location:
       location /protected/ {
         pow on;                 # provided by the module
         proxy_pass http://127.0.0.1:9000;
       }
     }
   }

3) Restart NGINX:
   sudo nginx -t && sudo systemctl reload nginx

EOF
