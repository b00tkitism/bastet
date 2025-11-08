use sha2::{Digest, Sha256};
use hmac::{Hmac, Mac};
use sha2::Sha256 as Sha2;
use base64::{engine::general_purpose as b64, Engine};
use once_cell::sync::OnceCell;
use rand::RngCore;
use std::ffi::CString;
use std::os::raw::{c_char, c_uchar, c_int, c_ulong};
use std::time::{SystemTime, UNIX_EPOCH, Duration};

type HmacSha256 = Hmac<Sha2>;

static SECRET: OnceCell<Vec<u8>> = OnceCell::new();

fn lz_bits(bytes: &[u8]) -> u16 {
    let mut bits = 0u16;
    for &b in bytes {
        if b == 0 { bits += 8; continue; }
        bits += b.leading_zeros() as u16;
        break;
    }
    bits
}

#[unsafe(no_mangle)]
pub extern "C" fn bastet_set_secret(ptr: *const c_uchar, len: c_ulong) -> c_int {
    if ptr.is_null() || len == 0 {
        return 0;
    }
    
    let bytes = unsafe { 
        std::slice::from_raw_parts(ptr, len as usize) 
    }.to_vec();
    let _ = SECRET.set(bytes);
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn bastet_issue_challenge(json_out: *mut *const c_char, difficulty_bits: u16, ttl_millis: u64) -> c_int {
    if json_out.is_null() { 
        return 0; 
    }

    let secret = match SECRET.get() { 
        Some(s) => s, 
        None => return 0 
    };

    let mut data = vec![0u8; 32];
    rand::rng().fill_bytes(&mut data);

    let expires_at = SystemTime::now()
        .checked_add(Duration::from_millis(ttl_millis)).unwrap()
        .duration_since(UNIX_EPOCH).unwrap().as_secs();

    let mut mac = HmacSha256::new_from_slice(secret).unwrap();
    mac.update(&data);
    mac.update(&expires_at.to_le_bytes());
    mac.update(&difficulty_bits.to_le_bytes());
    let sig = mac.finalize().into_bytes(); // 32

    let obj = json::object! {
        data: b64::URL_SAFE_NO_PAD.encode(&data),
        expires_at: expires_at,
        difficulty: difficulty_bits,
        sig: b64::URL_SAFE_NO_PAD.encode(&sig),
    };

    let cstr = match CString::new(obj.dump()) { 
        Ok(s) => s, 
        Err(_) => return 0 
    };

    unsafe { 
        *json_out = libc::strdup(cstr.as_ptr()); 
    }
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn bastet_validate_cookie(
    cookie_ptr: *const c_uchar,
    cookie_len: c_ulong,
) -> c_int {
    if cookie_ptr.is_null() || cookie_len == 0 {
        return 0; 
    }
    
    let secret = match SECRET.get() { 
        Some(s) => s,
        None => return 0 
    };

    let slice = unsafe { 
        std::slice::from_raw_parts(cookie_ptr, cookie_len as usize)
    };
    let raw = match b64::URL_SAFE_NO_PAD.decode(slice) { 
        Ok(v) => v, 
        Err(_) => return 0 
    };

    if raw.len() != 32 + 8 + 2 + 8 + 32 { 
        return 0; 
    }

    let (data, rest)   = raw.split_at(32);
    let (exp_b, rest)  = rest.split_at(8);
    let (dif_b, rest)  = rest.split_at(2);
    let (non_b, sig)   = rest.split_at(8);

    let expires_at     = u64::from_le_bytes(exp_b.try_into().unwrap());
    let difficulty_bits= u16::from_le_bytes(dif_b.try_into().unwrap());
    let nonce          = u64::from_le_bytes(non_b.try_into().unwrap());

    let mut mac = HmacSha256::new_from_slice(secret).unwrap();
    mac.update(data);
    mac.update(exp_b);
    mac.update(dif_b);
    if mac.verify_slice(sig).is_err() { 
        return 0; 
    }

    let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs();
    if now > expires_at { 
        return 0; 
    }

    let mut hasher = Sha256::new();
    hasher.update(data);
    hasher.update(&nonce.to_le_bytes());
    let h = hasher.finalize();
    if lz_bits(&h) < difficulty_bits {
        return 0; 
    }

    1
}
