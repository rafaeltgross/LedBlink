#!/usr/bin/env python3
"""Mini HTTP relay: recebe GET /send?subject=...&message=...&time=... do ESP32 e envia email via SMTP."""
import smtplib, email.message, urllib.parse
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime

SMTP_HOST = "mail.rgross.ch"
SMTP_PORT = 465
SMTP_USER = "kurt@rgross.ch"
SMTP_PASS = "truk123$$$"
EMAIL_TO  = "rafael@rgross.ch"
EMAIL_FROM_ALIAS = "SwissLedClock@rgross.ch"

class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{timestamp}] {fmt % args}")

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        params = urllib.parse.parse_qs(parsed.query)

        device    = params.get("device", ["Unknown"])[0]
        subject   = params.get("subject", ["Notification"])[0].replace("+", " ")
        message   = params.get("message", ["No message"])[0].replace("+", " ")
        time_str  = params.get("time", ["N/A"])[0]

        email_subject = f"[{device}] {subject}"
        email_body    = f"{message}\n\nTime: {time_str}"

        try:
            msg = email.message.EmailMessage()
            msg["From"]    = EMAIL_FROM_ALIAS
            msg["Sender"]  = SMTP_USER
            msg["To"]      = EMAIL_TO
            msg["Subject"] = email_subject
            msg.set_content(email_body)

            with smtplib.SMTP_SSL(SMTP_HOST, SMTP_PORT) as s:
                s.login(SMTP_USER, SMTP_PASS)
                s.send_message(msg)

            print(f"✓ Email sent: {email_subject}")
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"OK")
        except Exception as e:
            print(f"✗ Error: {e}")
            self.send_response(500)
            self.end_headers()
            self.wfile.write(str(e).encode())

if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", 9090), Handler)
    print("=" * 50)
    print("SwissLedClock Email Relay")
    print("=" * 50)
    print(f"Listening on http://0.0.0.0:9090")
    print(f"From: {EMAIL_FROM_ALIAS}")
    print(f"To: {EMAIL_TO}")
    print("=" * 50)
    server.serve_forever()
