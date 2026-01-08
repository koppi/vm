#!/usr/bin/env python3
"""
Simple HTTP server for testing Qt WebAssembly application.
Run this script from 'out' directory and open http://localhost:8000
"""

import http.server
import socketserver
import os
import sys
import mimetypes

# Add WASM MIME type
mimetypes.add_type("application/wasm", ".wasm")

# Change to directory containing this script
os.chdir(os.path.dirname(os.path.abspath(__file__)))

PORT = 8000


class WASMHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=".", **kwargs)

    def end_headers(self):
        # Enable SharedArrayBuffer for better performance if supported
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

    def do_GET(self):
        # Handle favicon.ico request
        if self.path == "/favicon.ico":
            self.send_response(204)
            self.end_headers()
            return

        # Default handling
        return super().do_GET()


if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), WASMHandler) as httpd:
        print(f"🚀 Qt WebAssembly Server")
        print(f"📁 Serving directory: {os.getcwd()}")
        print(f"🌐 Open http://localhost:{PORT}")
        print(f"📄 Available files:")

        # List available files
        for file in os.listdir("."):
            if file.endswith((".html", ".js", ".wasm")):
                size = os.path.getsize(file)
                print(f"   - {file} ({size:,} bytes)")

        print(f"\n🔄 Press Ctrl+C to stop the server")
        print(f"💡 Make sure both vm.js and vm.wasm are present!")
        print("-" * 50)

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n👋 Server stopped")
