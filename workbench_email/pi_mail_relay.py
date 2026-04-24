#!/usr/bin/env python3
"""Mini HTTP relay: recebe GET /send?time=... do ESP32 e envia email via SMTP."""
import smtplib, email.message, urllib.parse
from http.server import HTTPServer, BaseHTTPRequestHandler

SMTP_HOST = "mail.rgross.ch"
SMTP_PORT = 465
SMTP_USER = "kurt@rgross.ch"
SMTP_PASS = "truk123$$$"
EMAIL_TO  = "rafael@rgross.ch"

class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print(fmt % args)

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        params = urllib.parse.parse_qs(parsed.query)
        timestamp = params.get("time", ["desconhecido"])[0]
        device    = params.get("device", ["ESP32-S3"])[0]

        subject = f"Workbench - Horario: {timestamp}"
        body    = (f"Ola Rafael,\n\n"
                   f"Horario atual: {timestamp}\n\n"
                   f"Enviado pelo {device} via RG-IoT.\n")

        try:
            msg = email.message.EmailMessage()
            msg["From"]    = SMTP_USER
            msg["To"]      = EMAIL_TO
            msg["Subject"] = subject
            msg.set_content(body)

            with smtplib.SMTP_SSL(SMTP_HOST, SMTP_PORT) as s:
                s.login(SMTP_USER, SMTP_PASS)
                s.send_message(msg)

            print(f"Email enviado: {subject}")
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"OK")
        except Exception as e:
            print(f"Erro: {e}")
            self.send_response(500)
            self.end_headers()
            self.wfile.write(str(e).encode())

if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", 9090), Handler)
    print("Relay escutando em :8080")
    server.serve_forever()
