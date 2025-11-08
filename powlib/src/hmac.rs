type HmacSha256 = Hmac<Sha2>;


#[inline]
pub fn hmac256(secret: &[u8], parts: &[&[u8]]) -> HmacSha256{
    let mut mac = HmacSha256::new_from_slice(secret).expect("key ok");
    for p in parts { mac.update(p); }
    mac
}

#[inline]
pub fn hmac256_finalize(secret: &[u8], parts: &[&[u8]]) -> [u8; SIG_LEN] {
    hmac256(secret, parts).finalize().into_bytes().into()
}

#[inline]
pub fn hmac256_verify(secret: &[u8], parts: &[&[u8]], tag: &[u8]) -> bool {
    hmac256(secret, parts).verify_slice(tag).is_ok()
}