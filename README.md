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

- ⚙️ **Plug-and-play directive:** `bastet_toggle;`
- 🧠 **Stateless validation** — no Redis or shared memory
- 📦 **Dynamic NGINX module** — no full rebuild required
- 💡 **Embedded assets** — HTML and JS solver bundled automatically and can be customized
- 🧰 **Interactive build script** with CI-friendly flags
- 🔒 **No external dependencies**, no telemetry, no JavaScript libraries

---

## 🧪 Name & Inspiration

> Bastet — the Egyptian goddess of protection — guards your services from unwanted visitors, silently and efficiently.

Bastet’s idea were inspired by [Anubis](https://github.com/TecharoHQ/anubis) by TecharoHQ.

---

## 🧠 Example Configuration
```
load_module /path/to/modules/ngx_http_bastet_module.so;

events {}

http {
	bastet_allow_x_bastet; # by default X-Bastet header is not allowed
	bastet_mode exclusion; # default is inclusion, it means all of routes are protected by Bastet except ones which is toggled by bastet_toggle directive
	bastet_secret "change_this_to_a_long_random_secret"; # a random secret used for HMAC
	bastet_difficulty_bits 15; # bigger == longer time to load your websites;
	bastet_ttl 1h; # solved challenge allowance duration


	server {
		listen 9190;
		location /not-safe {
			return 200 'hey';
		}

		location / {
			bastet_toggle; # toggle bastet mode
		}
	}

}

```

---

# 🗺️ Roadmap / TODO
- [x] Support PoW token via X-Bastet request header (in addition to / instead of cookies) for better API compatibility.
- [x] Optional header allowance. (`bastet_allow_x_bastet` directive)
- [ ] Detect Selenium
- [x] Generalize powlib for easier implementation of new hash algorithms.
- [ ] Add options to use optional obfuscation for JS solver

> ⚠️ as this project is only used for rate limiting not defeating bots and scrapers but we have plans for that too.
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
