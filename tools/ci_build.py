#!/usr/bin/env python3
"""Build both wrapper editions, test the selected auth dialect, and package downloads."""

import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
EDITIONS = {"cstrike": "10", "svencoop": "225840"}


def run(*args):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True)


def package(build, platform, auth, game, version, output):
    name = f"raspad-loader-{game}-{platform}-x86-{auth}-{version}"
    loader = f"{game}.exe" if platform == "windows" else f"{game}_linux"
    client = "steamclient.dll" if platform == "windows" else "steamclient.so"
    suffix = ".zip" if platform == "windows" else ".tar.gz"
    output.mkdir(parents=True, exist_ok=True)
    archive = output / (name + suffix)
    with tempfile.TemporaryDirectory(prefix="raspad-loader-package-") as tmp:
        stage = Path(tmp)
        for binary in (loader, client):
            shutil.copy2(build / binary, stage / binary)
        if platform == "linux":
            (stage / loader).chmod(0o755)
        shutil.copy2(ROOT / "LICENSE", stage / "LICENSE")
        licenses = stage / "licenses"
        licenses.mkdir()
        shutil.copy2(ROOT / "lib/authum/lib/monocypher/LICENCE.md", licenses / "Monocypher.md")
        if platform == "windows":
            shutil.copy2(build / "_deps/opus-src/COPYING", licenses / "Opus.txt")
        config = stage / "config"
        config.mkdir()
        shutil.copy2(ROOT / "config/loader.cfg.example", config / "loader.cfg.example")
        engine = "hl.exe" if platform == "windows" else "hl_linux"
        runtime = ("No separate Visual C++ runtime is required.\n" if platform == "windows" else
                   "Built on Debian 12 for 32-bit x86 (glibc 2.36 or newer).\n"
                   "On Debian/Ubuntu install runtime libraries:\n"
                   "  sudo dpkg --add-architecture i386\n"
                   "  sudo apt update\n"
                   "  sudo apt install libopus0:i386 libasound2:i386 libcurl4:i386\n")
        (stage / "README.txt").write_text(
            f"Raspad Loader {version} / {game} / {platform} x86 / auth: {auth}\n\n"
            f"Extract these files beside {engine} in your game installation.\n"
            f"Start {loader}; it launches {engine} -game {game}.\n"
            "This archive contains the game wrapper and steamclient.\n"
            "Use the existing hl.exe / hl_linux, engine libraries and game data from your installation.\n"
            "Use only one auth variant in a game installation: steamclient has the same filename.\n\n"
            + runtime + "\n"
            "Vellum auth uses the per-user profile key. RevEmu 2013 uses the legacy machine identity.\n"
            "The auth dialect cannot be switched in the config; use the matching archive.\n\n"
            "Optional settings: copy config/loader.cfg.example to config/loader.cfg.\n"
            "No master configured means the Internet server list stays empty.\n"
            "Favorites/history use config/serverbrowser.vdf. Existing configs are not bundled.\n",
            encoding="utf-8")
        if platform == "windows":
            with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
                for item in sorted(stage.rglob("*")):
                    if item.is_file():
                        z.write(item, item.relative_to(stage).as_posix())
        else:
            with tarfile.open(archive, "w:gz") as tar:
                for item in sorted(stage.iterdir()):
                    tar.add(item, arcname=item.name)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_name(archive.name + ".sha256").write_text(
        f"{digest}  {archive.name}\n", encoding="ascii")
    print(f"Packaged {archive}", flush=True)
    return archive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=("windows", "linux"), required=True)
    parser.add_argument("--auth", choices=("vellum", "revemu2013"), required=True)
    args = parser.parse_args()
    if os.environ.get("GITHUB_REF_TYPE") == "tag":
        version = os.environ["GITHUB_REF_NAME"]
    else:
        sha = os.environ.get("GITHUB_SHA") or subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
        version = sha[:12]
    version = re.sub(r"[^a-zA-Z0-9._-]", "-", version)
    for game, appid in EDITIONS.items():
        build = ROOT / "build-ci" / args.platform / args.auth / game
        run("cmake", "-S", ROOT, "-B", build, "-G", "Ninja",
            "-DCMAKE_BUILD_TYPE=Release", "-DVELLUM_STANDALONE=OFF",
            "-DVELLUM_NO_SERVER_BROWSER=OFF",
            "-DVELLUM_AUTH_REVEMU2013=" + ("ON" if args.auth == "revemu2013" else "OFF"),
            f"-DVELLUM_NAME={game}", f"-DVELLUM_GAME={game}", f"-DVELLUM_APPID={appid}",
            *(["-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl"] if args.platform == "windows" else []))
        run("cmake", "--build", build, "--parallel", "2")
        run("cmake", "--build", build, "--target", "auth_ticket_smoke", "loader_args_smoke", "--parallel", "2")
        run(build / ("auth_ticket_smoke.exe" if args.platform == "windows" else "auth_ticket_smoke"))
        run(build / ("loader_args_smoke.exe" if args.platform == "windows" else "loader_args_smoke"))
        package(build, args.platform, args.auth, game, version, ROOT / "dist")


if __name__ == "__main__":
    main()
