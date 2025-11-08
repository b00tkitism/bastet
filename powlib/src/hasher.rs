use sha2::{Digest, Sha256};

pub trait Hasher {
    fn digest(data: &[&[u8]]) -> Vec<u8>;
}

pub struct BastetSha256;

impl Hasher for BastetSha256 {
    fn digest(datas: &[&[u8]]) -> Vec<u8> {
        let mut sha256 = Sha256::new();
        for d in datas { sha256.update(d); }
        sha256.finalize().to_vec()
    }
}
