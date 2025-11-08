use rand::RngCore;
use crate::hasher::Hasher;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

pub struct Challenge {
    pub data: [u8; 32],
    pub expires_at: u64,
    pub difficulty_bits: u16,
}

#[inline]
fn lz_bits(bytes: &[u8]) -> u16 {
    let mut bits = 0u16;
    for &b in bytes {
        if b == 0 { bits += 8; continue; }
        bits += b.leading_zeros() as u16;
        break;
    }
    bits
}

#[inline]
fn now_unix() -> Result<u64, &'static str> {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map_err(|_| "time error")
        .map(|d| d.as_secs())
}

impl Challenge {
    pub fn issue(ttl_millis: u64, difficulty_bits: u16) -> Challenge {
        let mut chal_data = [0u8; 32];
        rand::rng().fill_bytes(&mut chal_data);

        let expires_at = SystemTime::now()
            .checked_add(Duration::from_millis(ttl_millis))
            .unwrap() 
            .duration_since(UNIX_EPOCH).unwrap()
            .as_secs();

        Challenge { data: chal_data, expires_at, difficulty_bits }
    }

    pub fn from(data: [u8; 32], expires_at: u64, difficulty_bits: u16) -> Challenge {
        Challenge { data, expires_at, difficulty_bits }
    }

    pub fn validate<H: Hasher>(&self, nonce: u64) -> Result<(), &'static str> {
        if now_unix()? > self.expires_at {
            return Err("challenge expired");
        }

        let hash = H::digest(&[&self.data, &nonce.to_le_bytes()]);
        let lz = lz_bits(&hash);
        if lz >= self.difficulty_bits {
            Ok(())
        } else {
            Err("insufficient difficulty")
        }
    }
}
