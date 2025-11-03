# 🐱 Bastet — Proof-of-Work NGINX Module

> **Bastet** is a lightweight NGINX dynamic module that adds a browser-friendly **Proof-of-Work (PoW)** challenge before processing a request.  
> It protects your web services from bots, scrapers, and abuse — **without CAPTCHAs**, **without third-party services**, and with **zero user friction**.

---

## ✨ Overview

Bastet integrates directly with NGINX as a **dynamic module** to create and verify one-time PoW challenges.  
When a client hits a protected endpoint, Bastet returns an HTML + JS page that computes a small proof (e.g., SHA-256 hash with leading zeros).  
Once solved, the browser retries the request with a valid token — and NGINX lets it through.

---

## 🧩 Features

- ⚙️ **Plug-and-play directive:** `pow on;`
- 🧠 **Stateless validation** — no Redis or shared memory
- 📦 **Dynamic NGINX module** — no full rebuild required
- 💡 **Embedded assets** — HTML and JS solver bundled automatically and can be customized
- 🧰 **Interactive build script** with CI-friendly flags
- 🔒 **No external dependencies**, no telemetry, no JavaScript libraries

---

## 🧠 Example Configuration
```
load_module /path/to/modules/ngx_http_pow_module.so;

events {}

http {
    server {
        listen 8080;
        server_name localhost;

        pow_secret "change_this_to_a_long_random_secret";
      	pow_difficulty_bits 18;
      	pow_ttl 1h; # indicate how long a solved challenge can be used to access protected endpoints;
        
        # Protected route – requires Proof-of-Work
        location /protected/ {
            pow on;                     # Enable Bastet
            proxy_pass http://127.0.0.1:9000;
        }

        # Optional: public homepage for testing
        location / {
            root /var/www/html;
            index index.html;
        }
    }
}
```

---

## 💰 Support & Donations

If you’d like to support my work, you can donate using any of the following cryptocurrencies:

| Cryptocurrency | Address |
|----------------|----------|
| **Bitcoin (BTC)** | `bc1qaucj7tnafluwpkhk5wpg75ahdw6daujl0vsn7z` |
| **Ethereum (ETH)** | `0x72ca1a3c1023628647732c3c40b863f82df34039` |
| **BNB (BSC)** | `0x72ca1a3c1023628647732c3c40b863f82df34039` |
| **Solana (SOL)** | `EN5jDUFRMZYX1B9vvXMybgZsUqea5KBmYDA5exkRFCNy` |
| **Tron (TRX)** | `TDxMUBnF4hsBqM1eyn9pASYJzT9igGVT34` |

Every contribution helps — thank you for your support! 🙏
