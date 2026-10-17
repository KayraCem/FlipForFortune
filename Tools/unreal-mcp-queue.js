// Unreal MCP icin istek sirasi (queue) proxy'si.
// Sorun: Unreal'in MCP sunucusu ayni anda gelen iki istegi kaldiramiyor.
// Cozum: 127.0.0.1:8001'de kucuk bir proxy acar, POST isteklerini Unreal'a (8000)
// TEK TEK iletir, sonra mcp-remote'u bu proxy'ye baglar.
const http = require('http');
const path = require('path');
const { spawn } = require('child_process');

const UPSTREAM_PORT = 8000;
const PROXY_PORT = 8002;
const LOCK_TIMEOUT_MS = 30000;

const HOP = ['transfer-encoding','connection','keep-alive','proxy-connection','upgrade','te','trailer'];
function cleanHeaders(h) {
  const out = {};
  for (const [k, v] of Object.entries(h)) if (!HOP.includes(k.toLowerCase())) out[k] = v;
  return out;
}

let busy = false;
const queue = [];

function next() {
  if (busy || queue.length === 0) return;
  busy = true;
  const job = queue.shift();
  let released = false;
  const release = () => {
    if (released) return;
    released = true;
    clearTimeout(timer);
    busy = false;
    setImmediate(next);
  };
  const timer = setTimeout(release, LOCK_TIMEOUT_MS);
  job(release);
}

function forward(req, res, done) {
  const up = http.request(
    { host: '127.0.0.1', port: UPSTREAM_PORT, method: req.method, path: req.url, headers: req.headers },
    (upRes) => {
      res.writeHead(upRes.statusCode, cleanHeaders(upRes.headers));
      upRes.pipe(res);
      upRes.on('end', done);
      upRes.on('close', done);
      upRes.on('error', done);
    }
  );
  up.on('error', (e) => {
    if (!res.headersSent) res.writeHead(502, { 'content-type': 'text/plain' });
    res.end('upstream error: ' + e.message);
    done();
  });
  req.pipe(up);
}

function startProxy() {
  const server = http.createServer((req, res) => {
    if (req.method === 'POST') {
      // Govdeyi once oku ki istek kuyrukta beklerken kaybolmasin
      const chunks = [];
      req.on('data', (c) => chunks.push(c));
      req.on('end', () => {
        const body = Buffer.concat(chunks);
        queue.push((release) => {
          const up = http.request(
            { host: '127.0.0.1', port: UPSTREAM_PORT, method: 'POST', path: req.url,
              headers: { ...req.headers, 'content-length': body.length } },
            (upRes) => {
              res.writeHead(upRes.statusCode, cleanHeaders(upRes.headers));
              upRes.pipe(res);
              upRes.on('end', release);
              upRes.on('close', release);
              upRes.on('error', release);
            }
          );
          up.on('error', (e) => {
            if (!res.headersSent) res.writeHead(502, { 'content-type': 'text/plain' });
            res.end('upstream error: ' + e.message);
            release();
          });
          up.end(body);
        });
        next();
      });
    } else {
      forward(req, res, () => {});
    }
  });
  server.on('error', () => { /* port baska bir kopyada aciksa sorun yok */ });
  server.listen(PROXY_PORT, '127.0.0.1');
  return server;
}

// Proxy'yi baslat; port doluysa (diger kopya calistiriyorsa) her 2 sn'de bir tekrar dene,
// boylece o kopya kapanirsa bu kopya devralir.
let server = startProxy();
setInterval(() => { if (!server.listening) server = startProxy(); }, 2000);

// mcp-remote'u proxy'ye bagla
const npx = path.join(path.dirname(process.execPath), 'npx.cmd');
const child = spawn(`"${npx}" -y mcp-remote http://127.0.0.1:${PROXY_PORT}/mcp`, {
  stdio: 'inherit',
  shell: true,
});
child.on('exit', (code) => process.exit(code ?? 0));
process.on('SIGTERM', () => child.kill());
process.on('SIGINT', () => child.kill());
