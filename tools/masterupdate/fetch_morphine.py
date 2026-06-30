#!/usr/bin/env python3
"""Download latest offsets.hpp, offsets.json, and materials.hpp from Morphine.

Exit codes:
  0 = success (at least offsets.hpp downloaded)
  1 = partial failure (offsets.hpp downloaded, some others failed)
  2 = Morphine offline (all URLs unreachable)
"""
import json
import os
import socket
import sys
import time
import urllib.request
from pathlib import Path
from urllib.error import URLError, HTTPError

CONFIG_PATH = Path(__file__).parent / "config.json"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def is_morphine_reachable(url: str, timeout: int = 5) -> bool:
    """Quick TCP connectivity check — is the host even listening?"""
    try:
        from urllib.parse import urlparse
        parsed = urlparse(url)
        host = parsed.hostname
        port = parsed.port or (443 if parsed.scheme == "https" else 80)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        result = sock.connect_ex((host, port))
        sock.close()
        return result == 0
    except Exception:
        return False


def fetch(url: str, dest: Path, retries: int = 2, backoff: float = 2.0) -> str:
    """Fetch URL with retry+backoff on timeout."""
    last_err = None
    for attempt in range(1, retries + 1):
        try:
            print(f"[FETCH] {url} (attempt {attempt}/{retries})")
            req = urllib.request.Request(url, headers={"User-Agent": "BeaztMasterUpdate/1.0"})
            with urllib.request.urlopen(req, timeout=15) as resp:
                data = resp.read().decode("utf-8", errors="ignore")
            dest.parent.mkdir(parents=True, exist_ok=True)
            with open(dest, "w", encoding="utf-8") as f:
                f.write(data)
            print(f"[OK] Saved {dest} ({len(data)} chars)")
            return data
        except (URLError, HTTPError, socket.timeout, ConnectionError) as e:
            last_err = e
            err_type = type(e).__name__
            if "Name or service not known" in str(e) or "getaddrinfo" in str(e).lower():
                print(f"[DNS] Cannot resolve host — Morphine likely offline")
                return None
            if attempt < retries:
                wait = backoff * (2 ** (attempt - 1))
                print(f"[RETRY] {err_type}: {e} - waiting {wait:.0f}s")
                time.sleep(wait)
            else:
                print(f"[FAIL] {err_type}: {e} (all {retries} attempts exhausted)")
        except Exception as e:
            last_err = e
            if attempt < retries:
                wait = backoff * (2 ** (attempt - 1))
                print(f"[RETRY] {type(e).__name__}: {e} - waiting {wait:.0f}s")
                time.sleep(wait)
            else:
                print(f"[FAIL] {type(e).__name__}: {e} (all {retries} attempts exhausted)")
    return None


def main():
    cfg = load_config()
    out_dir = Path(__file__).parent / "output"
    out_dir.mkdir(parents=True, exist_ok=True)

    urls = {
        "offsets.hpp": cfg["morphine_offsets_url"],
        "offsets.json": cfg.get("morphine_offsets_json_url", ""),
        "materials.hpp": cfg.get("morphine_materials_url", ""),
    }

    # Quick connectivity check — if host is unreachable, fail fast
    primary_url = urls["offsets.hpp"]
    print(f"[CHECK] Testing connectivity to Morphine...")
    if not is_morphine_reachable(primary_url, timeout=5):
        print(f"[OFFLINE] Morphine host is unreachable (TCP connect failed)")
        print(f"[OFFLINE] All Morphine URLs will be skipped.")
        print(f"[OFFLINE] Pipeline will use sha-dumper data only.")
        sys.exit(2)

    print(f"[CHECK] Morphine host is reachable, proceeding with fetch...")

    success_count = 0
    failure_count = 0

    for name, url in urls.items():
        if not url:
            continue
        dest = out_dir / name
        data = fetch(url, dest, retries=2, backoff=2.0)
        if data is not None:
            if name == "offsets.hpp":
                success_count += 1
        else:
            failure_count += 1
            if name == "offsets.hpp":
                print(f"[ERROR] offsets.hpp fetch failed — this is the primary source.")
            elif name == "materials.hpp":
                dest.write_text("", encoding="utf-8")

    if success_count == 0:
        # offsets.hpp failed — determine if it's an offline situation
        if failure_count == len([u for u in urls.values() if u]):
            print(f"[OFFLINE] All {failure_count} Morphine URLs failed — Morphine is offline.")
            sys.exit(2)
        else:
            print(f"[ERROR] offsets.hpp not downloaded.")
            sys.exit(1)

    print(f"[OK] Fetch complete ({success_count} primary, {failure_count} failed).")
    sys.exit(0)


if __name__ == "__main__":
    main()
