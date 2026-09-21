"""Real local HTTPS fixture; fake credentials only, no external calls."""
import http.server
import ssl
import subprocess
import sys
import threading
from pathlib import Path

root=Path(__file__).resolve().parents[1]
cert=root/'.cache/redirect-test/cert.pem'
key=cert.with_name('key.pem')
cert.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-days','1',
 '-keyout',str(key),'-out',str(cert),'-subj','/CN=localhost',
 '-addext','subjectAltName=DNS:localhost,IP:127.0.0.1'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
visits=[]
class Server(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args): pass
 def do_POST(self):
  visits.append(self.path)
  body=self.rfile.read(int(self.headers.get('Content-Length','0')))
  assert body==b'password=fixture-password'
  if self.path=='/login' and 'setup=ready' in self.headers.get('Cookie',''):
   assert 'remix_userkey=fixture-session' in self.headers.get('Cookie','')
   self.send_response(200);self.end_headers();self.wfile.write(b'{"success":1}');return
  self.send_response(307)
  if self.path=='/login':
   self.send_header('Set-Cookie','setup=ready; Path=/; Secure; HttpOnly')
   target='/login'
  elif self.path=='/cross-host': target=f'https://127.0.0.1:{self.server.server_port}/stolen'
  elif self.path=='/downgrade': target=f'http://localhost:{self.server.server_port}/stolen'
  else: target='/loop'
  self.send_header('Location',target);self.end_headers();self.wfile.write(b'Intermediate redirect body, not JSON')
server=http.server.HTTPServer(('127.0.0.1',0),Server)
ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(cert,key)
server.socket=ctx.wrap_socket(server.socket,server_side=True)
threading.Thread(target=server.serve_forever,daemon=True).start()
result=subprocess.run([str(root/'build/test_redirect'),f'https://localhost:{server.server_port}',str(cert)])
server.shutdown()
if result.returncode: sys.exit(1)
assert visits.count('/login')==2,visits
assert '/stolen' not in visits,visits
assert visits.count('/loop')<=6,visits
