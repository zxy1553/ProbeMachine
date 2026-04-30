from __future__ import annotations

import json
import mimetypes
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

from demo.runner import DemoParams, run_demo


ROOT = Path(__file__).resolve().parent
STATIC_DIR = ROOT / "demo" / "static"
BACKGROUND_DIR = STATIC_DIR / "backgrounds"
BACKGROUND_FILES = {
    "bg-content.png": Path("C:/Users/ziyadyao/Downloads/bg-content.png"),
    "bg-section.png.png": Path("C:/Users/ziyadyao/Downloads/bg-section.png.png"),
    "bg-cover.png": Path("C:/Users/ziyadyao/Downloads/bg-cover.png"),
}


def read_int(params: dict[str, list[str]], name: str, default: int) -> int:
    try:
        return int(params.get(name, [default])[0])
    except (TypeError, ValueError):
        return default


def resolve_background(name: str) -> Path | None:
    project_copy = BACKGROUND_DIR / name
    if project_copy.exists():
        return project_copy
    return BACKGROUND_FILES.get(name)


class DemoHandler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/run":
            query = parse_qs(parsed.query)
            params = DemoParams(
                state_count=read_int(query, "state_count", 5),
                candidate_count=read_int(query, "candidate_count", 2),
                voter_count=read_int(query, "voter_count", 1),
                agent_count=read_int(query, "agent_count", 1),
            )
            self.send_json(run_demo(params))
            return

        if parsed.path.startswith("/backgrounds/"):
            self.send_background(parsed.path.removeprefix("/backgrounds/"))
            return

        if parsed.path in {"", "/"}:
            self.send_static("index.html")
            return

        self.send_static(parsed.path.lstrip("/"))

    def send_json(self, payload: dict) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_background(self, name: str) -> None:
        path = resolve_background(name)
        if path is None or not path.exists() or not path.is_file():
            self.send_error(404)
            return

        body = path.read_bytes()
        content_type = mimetypes.guess_type(str(path))[0] or "image/png"
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_static(self, relative_path: str) -> None:
        path = (STATIC_DIR / relative_path).resolve()
        if STATIC_DIR.resolve() not in path.parents and path != STATIC_DIR.resolve():
            self.send_error(403)
            return
        if not path.exists() or not path.is_file():
            self.send_error(404)
            return

        body = path.read_bytes()
        content_type = mimetypes.guess_type(str(path))[0] or "application/octet-stream"
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format: str, *args) -> None:
        print("[ProbeMachine demo]", format % args)


def main() -> None:
    host = "127.0.0.1"
    port = 8765
    server = ThreadingHTTPServer((host, port), DemoHandler)
    url = f"http://{host}:{port}"
    print(f"ProbeMachine demo server running at {url}")
    print("Press Ctrl+C to stop.")
    try:
        webbrowser.open(url)
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nDemo server stopped.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
