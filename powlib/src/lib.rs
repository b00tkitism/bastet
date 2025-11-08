use base64::{engine::general_purpose as b64, Engine};
use once_cell::sync::OnceCell;
use std::ffi::CString;
use std::os::raw::{c_char, c_uchar, c_int, c_ulong};

use crate::hmac::{*};
use crate::constants::{*};
use crate::challenge::Challenge;
use crate::hasher::BastetSha256;

pub mod hmac;
pub mod hasher;
pub mod constants;
pub mod challenge;

static SECRET: OnceCell<Vec<u8>> = OnceCell::new();

#[unsafe(no_mangle)]
pub extern "C" fn bastet_set_secret(ptr: *const c_uchar, len: c_ulong) -> c_int {
    if ptr.is_null() || len == 0 { return 0; }
    let bytes = unsafe { std::slice::from_raw_parts(ptr, len as usize) }.to_vec();
    let _ = SECRET.set(bytes);
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn bastet_issue_challenge(
    json_out: *mut *mut c_char,
    difficulty_bits: u16,
    ttl_millis: u64
) -> c_int {
    if json_out.is_null() { return 0; }

    let secret = match SECRET.get() { Some(s) => s, None => return 0 };

    let ch = Challenge::issue(ttl_millis, difficulty_bits);

    let sig = hmac256(
        secret,
        &[ &ch.data, &ch.expires_at.to_le_bytes(), &ch.difficulty_bits.to_le_bytes() ],
    );

    let obj = json::object! {
        data:        b64::URL_SAFE_NO_PAD.encode(&ch.data),
        expires_at:  ch.expires_at,
        difficulty:  difficulty_bits,
        sig:         b64::URL_SAFE_NO_PAD.encode(&sig),
    };

    let s = match CString::new(obj.dump()) { Ok(s) => s, Err(_) => return 0 };
    unsafe { *json_out = s.into_raw(); }
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn bastet_validate_cookie(
    cookie_ptr: *const c_uchar,
    cookie_len: c_ulong,
) -> c_int {
    if cookie_ptr.is_null() || cookie_len == 0 { return 0; }
    let secret = match SECRET.get() { Some(s) => s, None => return 0 };

    let slice = unsafe { std::slice::from_raw_parts(cookie_ptr, cookie_len as usize) };
    let raw = match b64::URL_SAFE_NO_PAD.decode(slice) { Ok(v) => v, Err(_) => return 0 };

    if raw.len() != RAW_LEN { return 0; }

    let (data, rest)  = raw.split_at(DATA_LEN);
    let (exp_b, rest) = rest.split_at(EXP_LEN);
    let (dif_b, rest) = rest.split_at(DIF_LEN);
    let (non_b, sig)  = rest.split_at(NON_LEN);

    if !hmac256_verify(secret, &[data, exp_b, dif_b], sig) { return 0; }

    let expires_at      = u64::from_le_bytes(exp_b.try_into().unwrap());
    let difficulty_bits = u16::from_le_bytes(dif_b.try_into().unwrap());
    let nonce           = u64::from_le_bytes(non_b.try_into().unwrap());

    let ch = Challenge::from(data.try_into().unwrap(), expires_at, difficulty_bits);
    match ch.validate::<BastetSha256>(nonce) {
        Ok(_) => 1,
        Err(_) => 0,
    }
}
