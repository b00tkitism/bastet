(function(){
function b64u_bytes(u8){
  let s=''; for(let i=0;i<u8.length;i++) s+=String.fromCharCode(u8[i]);
  return btoa(s).replaceAll('+','-').replaceAll('/','_').replaceAll('=','');
}
function b64u_dec(s){
  s=s.replace(/-/g,'+').replace(/_/g,'/');
  const pad=s.length%4; if(pad){s+='='.repeat(4-pad);}
  const bin=atob(s); const u8=new Uint8Array(bin.length);
  for(let i=0;i<bin.length;i++) u8[i]=bin.charCodeAt(i);
  return u8;
}
function lzBits(arr){
  let bits=0;
  for(const b of arr){
    if(b===0){bits+=8; continue;}
    for(let i=7;i>=0;i--){ if((b>>i)&1) return bits; bits++; }
    break;
  }
  return bits;
}
async function solve(){
  const el=document.getElementById('bastet-challenge');
  if(!el) return;
  const challenge = JSON.parse(el.textContent || '{}');
  const data=b64u_dec(challenge.data);
  const sig=b64u_dec(challenge.sig);
  const exp=BigInt(challenge.expires_at);
  const diff=BigInt(challenge.difficulty);
  let nonce=0n;
  for(;;){
    const buf=new ArrayBuffer(data.length+8);
    const dv=new DataView(buf); const u8=new Uint8Array(buf);
    u8.set(data,0); dv.setBigUint64(data.length, nonce, true);
    const dig=await crypto.subtle.digest('SHA-256', u8);
    if(lzBits(new Uint8Array(dig))>=Number(diff)){
      const out=new Uint8Array(32+8+2+8+32);
      out.set(data,0);
      const dv2=new DataView(out.buffer);
      dv2.setBigUint64(32, exp, true);
      dv2.setUint16(40, Number(diff), true);
      dv2.setBigUint64(42, nonce, true);
      out.set(sig, 50);
      document.cookie='bastet='+b64u_bytes(out)+'; Max-Age=3600; Path=/; SameSite=Lax';
      location.replace(location.href);
      return;
    }
    nonce++;
  }
}
solve().catch(()=>{});
})();
