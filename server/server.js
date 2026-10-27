import express from 'express';
import http from 'http';
import { Server } from 'socket.io';
import cors from 'cors';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, '..');

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
  cors: { origin: '*', methods: ['GET', 'POST'] }
});

app.use(cors());
app.use(express.json());

// Serve Stitch Static Dashboards
app.use('/live', express.static(path.join(rootDir, 'aegishelm_live_cockpit_dashboard')));
app.use('/logs', express.static(path.join(rootDir, 'aegishelm_trip_logs_blackbox_scorecard')));
app.use(express.static(rootDir));

// Root route redirects to live cockpit
app.get('/', (req, res) => {
  res.sendFile(path.join(rootDir, 'aegishelm_live_cockpit_dashboard', 'code.html'));
});

app.get('/trip-logs', (req, res) => {
  res.sendFile(path.join(rootDir, 'aegishelm_trip_logs_blackbox_scorecard', 'code.html'));
});

// Hardware COM Port API Endpoints
app.get('/api/ports', async (req, res) => {
  if (!SerialPort) return res.json({ available: false, ports: [] });
  try {
    const ports = await SerialPort.list();
    res.json({ available: true, active: connectedPortName, mode: systemMode, ports });
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

app.post('/api/connect', (req, res) => {
  const { port } = req.body;
  if (!port) return res.status(400).json({ error: 'Port path required' });
  pauseAutoConnectUntil = 0;
  const ok = connectToSerial(port);
  res.json({ success: ok, port });
});

app.post('/api/disconnect', (req, res) => {
  const pauseMs = req.body.pauseMs || 30000;
  disconnectSerial(pauseMs);
  res.json({ success: true, message: `Disconnected. Auto-reconnect paused for ${pauseMs / 1000}s` });
});

const PORT = process.env.PORT || 8080;

// Dynamic import for serialport
let SerialPort = null;
let ReadlineParser = null;
let activeSerialPort = null;

try {
  const serialModule = await import('serialport');
  const parserModule = await import('@serialport/parser-readline');
  SerialPort = serialModule.SerialPort;
  ReadlineParser = parserModule.ReadlineParser;
  console.log('[Hardware] SerialPort module initialized successfully.');
} catch (e) {
  console.log('[Hardware] Running in Virtual Simulator mode.');
}

let systemMode = 'SIMULATOR';
let connectedPortName = null;

let currentTelemetry = {
  helmet: 1,
  alcohol: 0,
  drowsy: 0,
  crash: 0,
  ax: 0.04,
  ay: 0.02,
  az: 0.98,
  gForce: 0.99,
  pitch: 2.1,
  roll: 0.8,
  relay: 1,
  override: 0,
  speedKmh: 42,
  lcd1: "HELMET: SAFE OK ",
  lcd2: "ENGINE: STARTED ",
  timestamp: Date.now()
};

let simConfig = {
  helmet: true,
  alcohol: false,
  drowsy: false,
  crash: false,
  manualCutoff: false,
  pitch: 2.0,
  roll: 0.8
};

// Serial Connection
let pauseAutoConnectUntil = 0;

function disconnectSerial(pauseMs = 30000) {
  pauseAutoConnectUntil = Date.now() + pauseMs;
  if (activeSerialPort && activeSerialPort.isOpen) {
    try { activeSerialPort.close(); } catch (e) {}
  }
  connectedPortName = null;
  systemMode = 'SIMULATOR';
  io.emit('system-status', { 
    mode: systemMode, 
    port: null, 
    paused: true, 
    pauseRemaining: Math.round(pauseMs / 1000) 
  });
  console.log(`[Hardware] Serial disconnected. Auto-reconnect paused for ${pauseMs / 1000}s (allows Arduino IDE firmware upload).`);
}

async function autoConnectArduino() {
  if (!SerialPort) return;
  if (Date.now() < pauseAutoConnectUntil) return;
  if (activeSerialPort && activeSerialPort.isOpen) return;

  try {
    const ports = await SerialPort.list();
    if (!ports || ports.length === 0) return;

    // Filter candidate ports:
    // Priority 1: Ports with 'arduino', 'ch340', 'ch341', 'wch', 'ftdi', 'cp210' or known Vendor IDs
    const knownVendors = ['2341', '1a86', '0403', '10c4', '067b'];
    let arduinoPort = ports.find(p => {
      const mf = (p.manufacturer || '').toLowerCase();
      const fn = (p.friendlyName || '').toLowerCase();
      const pnp = (p.pnpId || '').toLowerCase();
      const vid = (p.vendorId || '').toLowerCase();
      return mf.includes('arduino') || fn.includes('arduino') ||
             mf.includes('wch') || fn.includes('ch340') || fn.includes('ch341') ||
             mf.includes('ftdi') || fn.includes('ftdi') ||
             mf.includes('silicon') || fn.includes('cp210') ||
             pnp.includes('ch340') || pnp.includes('usb') ||
             knownVendors.includes(vid);
    });

    // Priority 2: Any COM port other than motherboard header COM1
    if (!arduinoPort) {
      arduinoPort = ports.find(p => {
        const path = (p.path || '').toUpperCase();
        return path.startsWith('COM') && path !== 'COM1';
      });
    }

    // Priority 3: Fallback to any detected COM port
    if (!arduinoPort && ports.length > 0) {
      arduinoPort = ports[0];
    }

    if (arduinoPort) {
      console.log(`[Hardware] Auto-connecting to Arduino on ${arduinoPort.path} (${arduinoPort.friendlyName || arduinoPort.manufacturer || 'Serial Device'})`);
      connectToSerial(arduinoPort.path);
    }
  } catch (err) {
    // Port scan error silent catch
  }
}

// Background auto-scan every 3 seconds if disconnected and not paused
setInterval(() => {
  if (systemMode === 'SIMULATOR' || !activeSerialPort || !activeSerialPort.isOpen) {
    autoConnectArduino();
  }
}, 3000);

function connectToSerial(portPath, baudRate = 9600) {
  if (!SerialPort) return false;
  if (activeSerialPort && activeSerialPort.isOpen) {
    try { activeSerialPort.close(); } catch (e) {}
  }

  try {
    activeSerialPort = new SerialPort({ path: portPath, baudRate });
    // Use '\n' delimiter to handle both LF and CRLF line endings
    const parser = activeSerialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

    activeSerialPort.on('open', () => {
      console.log(`[Hardware] Live serial connection established on ${portPath}`);
      connectedPortName = portPath;
      systemMode = 'HARDWARE';
      io.emit('system-status', { mode: systemMode, port: connectedPortName });
    });

    parser.on('data', (line) => {
      const trimmed = line.trim();
      if (!trimmed) return;

      // 1. JSON Telemetry Packet (Full MPU-6050 & sensor data)
      if (trimmed.startsWith('{') && trimmed.endsWith('}')) {
        try {
          const parsed = JSON.parse(trimmed);
          currentTelemetry = {
            ...currentTelemetry,
            ...parsed,
            timestamp: Date.now()
          };
          if (systemMode !== 'HARDWARE') {
            systemMode = 'HARDWARE';
            io.emit('system-status', { mode: systemMode, port: connectedPortName });
          }
          io.emit('telemetry', currentTelemetry);
          return;
        } catch (err) {}
      }

      // 2. Direct RF Packet string parsing from Helmet-main (Blink.ino / Receiver.ino)
      if (trimmed.includes('[SAFE]')) {
        currentTelemetry = {
          ...currentTelemetry,
          helmet: 1,
          alcohol: 0,
          drowsy: 0,
          crash: 0,
          relay: 1,
          override: 0,
          speedKmh: 42,
          lcd1: "HELMET: SAFE OK ",
          lcd2: "ENGINE: STARTED ",
          timestamp: Date.now()
        };
        if (systemMode !== 'HARDWARE') {
          systemMode = 'HARDWARE';
          io.emit('system-status', { mode: systemMode, port: connectedPortName });
        }
        io.emit('telemetry', currentTelemetry);
      } else if (trimmed.includes('[ALCOHOL]')) {
        currentTelemetry = {
          ...currentTelemetry,
          helmet: 1,
          alcohol: 1,
          relay: 0,
          speedKmh: 0,
          lcd1: "ALCOHOL DETECTED",
          lcd2: "ENGINE: LOCKED! ",
          timestamp: Date.now()
        };
        if (systemMode !== 'HARDWARE') {
          systemMode = 'HARDWARE';
          io.emit('system-status', { mode: systemMode, port: connectedPortName });
        }
        io.emit('telemetry', currentTelemetry);
      } else if (trimmed.includes('[NO_HELMET]')) {
        currentTelemetry = {
          ...currentTelemetry,
          helmet: 0,
          alcohol: 0,
          relay: 0,
          speedKmh: 0,
          lcd1: "WEAR HELMET &   ",
          lcd2: "FASTEN STRAP!   ",
          timestamp: Date.now()
        };
        if (systemMode !== 'HARDWARE') {
          systemMode = 'HARDWARE';
          io.emit('system-status', { mode: systemMode, port: connectedPortName });
        }
        io.emit('telemetry', currentTelemetry);
      } else if (trimmed.includes('[CRASH]')) {
        currentTelemetry = {
          ...currentTelemetry,
          crash: 1,
          gForce: 4.35,
          pitch: 64.0,
          roll: -52.0,
          relay: 0,
          speedKmh: 0,
          lcd1: "CRASH DETECTED! ",
          lcd2: "SOS DISPATCHING ",
          timestamp: Date.now()
        };
        if (systemMode !== 'HARDWARE') {
          systemMode = 'HARDWARE';
          io.emit('system-status', { mode: systemMode, port: connectedPortName });
        }
        io.emit('telemetry', currentTelemetry);
      } else if (trimmed.toLowerCase().includes('signal lost') || trimmed.includes('TIMEOUT')) {
        currentTelemetry = {
          ...currentTelemetry,
          helmet: 0,
          relay: 0,
          speedKmh: 0,
          lcd1: "SIGNAL LOST     ",
          lcd2: "ENGINE STOPPED  ",
          timestamp: Date.now()
        };
        if (systemMode !== 'HARDWARE') {
          systemMode = 'HARDWARE';
          io.emit('system-status', { mode: systemMode, port: connectedPortName });
        }
        io.emit('telemetry', currentTelemetry);
      }
    });

    activeSerialPort.on('close', () => {
      console.log(`[Hardware] Serial port ${connectedPortName || portPath} closed. Reverting to Virtual Simulator.`);
      connectedPortName = null;
      systemMode = 'SIMULATOR';
      io.emit('system-status', { mode: systemMode, port: null });
    });

    activeSerialPort.on('error', (err) => {
      console.error('[Hardware] Serial Error:', err.message);
      connectedPortName = null;
      systemMode = 'SIMULATOR';
      io.emit('system-status', { mode: systemMode, port: null });
    });

    return true;
  } catch (err) {
    return false;
  }
}

function sendSerialCommand(cmd) {
  if (activeSerialPort && activeSerialPort.isOpen) {
    activeSerialPort.write(`${cmd}\n`);
    console.log(`[Hardware] Sent command to Arduino: ${cmd}`);
    return true;
  }
  return false;
}

// 10Hz Simulation Loop
let simTime = 0;
setInterval(() => {
  if (systemMode !== 'SIMULATOR') return;

  simTime += 0.1;
  const noiseX = (Math.sin(simTime * 2.5) * 0.08) + (Math.random() - 0.5) * 0.04;
  const noiseY = (Math.cos(simTime * 1.8) * 0.06) + (Math.random() - 0.5) * 0.03;
  const noiseZ = 0.98 + (Math.sin(simTime * 4.0) * 0.05);

  let ax = parseFloat(noiseX.toFixed(2));
  let ay = parseFloat(noiseY.toFixed(2));
  let az = parseFloat(noiseZ.toFixed(2));
  let gForce = parseFloat(Math.sqrt(ax * ax + ay * ay + az * az).toFixed(2));

  let crashFlag = 0;
  if (simConfig.crash) {
    crashFlag = 1;
    gForce = 4.35;
    ax = 3.20;
    ay = -2.10;
    az = 1.95;
    simConfig.pitch = 64.2;
    simConfig.roll = -52.8;
  }

  const h = simConfig.helmet ? 1 : 0;
  const a = simConfig.alcohol ? 1 : 0;
  const d = simConfig.drowsy ? 1 : 0;
  const relay = (h === 1 && a === 0 && crashFlag === 0 && !simConfig.manualCutoff) ? 1 : 0;

  let lcd1 = "HELMET: SAFE OK ";
  let lcd2 = "ENGINE: STARTED ";

  if (crashFlag === 1) {
    lcd1 = "CRASH DETECTED! ";
    lcd2 = "SOS DISPATCHING ";
  } else if (simConfig.manualCutoff) {
    lcd1 = "REMOTE OVERRIDE ";
    lcd2 = "IGNITION CUT!   ";
  } else if (a === 1) {
    lcd1 = "ALCOHOL DETECTED";
    lcd2 = "ENGINE: LOCKED! ";
  } else if (h === 0) {
    lcd1 = "WEAR HELMET &   ";
    lcd2 = "FASTEN STRAP!   ";
  } else if (d === 1) {
    lcd1 = "DROWSINESS ALERT";
    lcd2 = "TAKE A BREAK!   ";
  }

  currentTelemetry = {
    helmet: h,
    alcohol: a,
    drowsy: d,
    crash: crashFlag,
    ax,
    ay,
    az,
    gForce,
    pitch: simConfig.pitch,
    roll: simConfig.roll,
    relay,
    override: simConfig.manualCutoff ? 1 : 0,
    speedKmh: relay === 1 ? 42 : 0,
    lcd1,
    lcd2,
    timestamp: Date.now()
  };

  io.emit('telemetry', currentTelemetry);
}, 100);

// Socket.io
io.on('connection', (socket) => {
  console.log(`[Socket] Stitch Client connected: ${socket.id}`);
  socket.emit('telemetry', currentTelemetry);
  socket.emit('system-status', { mode: systemMode, port: connectedPortName });

  socket.on('set-state', (newState) => {
    switch (newState) {
      case 'safe':
        simConfig = { helmet: true, alcohol: false, drowsy: false, crash: false, manualCutoff: false, pitch: 0, roll: 0 };
        break;
      case 'helmet-off':
        simConfig = { ...simConfig, helmet: false, crash: false };
        break;
      case 'alcohol':
        simConfig = { ...simConfig, alcohol: true, crash: false };
        break;
      case 'drowsy':
        simConfig = { ...simConfig, drowsy: true, crash: false, pitch: 18, roll: 4 };
        break;
      case 'crash':
        simConfig = { ...simConfig, crash: true };
        break;
    }
  });

  socket.on('toggle-engine', () => {
    simConfig.manualCutoff = !simConfig.manualCutoff;
    sendSerialCommand(simConfig.manualCutoff ? 'LOCK' : 'UNLOCK');
  });

  socket.on('set-attitude', (data) => {
    if (data.pitch !== undefined) simConfig.pitch = data.pitch;
    if (data.roll !== undefined) simConfig.roll = data.roll;
  });

  socket.on('get-ports', async () => {
    if (!SerialPort) return socket.emit('ports-list', []);
    try {
      const ports = await SerialPort.list();
      socket.emit('ports-list', ports);
    } catch (e) {
      socket.emit('ports-list', []);
    }
  });

  socket.on('select-port', (portPath) => {
    pauseAutoConnectUntil = 0;
    connectToSerial(portPath);
  });

  socket.on('release-port', (pauseMs) => {
    disconnectSerial(pauseMs || 30000);
  });

  // Cloud SMS & Emergency SOS Dispatch Handler
  socket.on('dispatch-sos', async (sosData) => {
    console.log(`\n=======================================================`);
    console.log(`🚨 [EMERGENCY DISPATCH INITIATED - HIGH-G IMPACT / FALL]`);
    console.log(`Timestamp:         ${new Date(sosData.timestamp || Date.now()).toISOString()}`);
    console.log(`Peak G-Force:      ${sosData.gForce || '4.35'}G`);
    console.log(`Live GPS Location: Lat ${sosData.lat}, Lon ${sosData.lon}`);
    console.log(`Google Maps Route: ${sosData.mapsUrl || `https://www.google.com/maps?q=${sosData.lat},${sosData.lon}`}`);
    console.log(`Emergency Contact: ${sosData.emergencyContact || 'EMS 108'}`);
    console.log(`=======================================================`);

    const alertMessage = `🚨 CRITICAL SOS: AegisHelm motorcycle crash detected! G-Force: ${sosData.gForce || '4.35'}G. Live Location: ${sosData.mapsUrl}. Contact: ${sosData.emergencyContact}`;

    let smsSuccess = false;
    let providerName = 'Simulated Cloud Gateway';

    // 1. Live Twilio Cloud SMS (If environment credentials provided)
    if (process.env.TWILIO_ACCOUNT_SID && process.env.TWILIO_AUTH_TOKEN && process.env.TWILIO_PHONE_NUMBER) {
      try {
        const auth = Buffer.from(`${process.env.TWILIO_ACCOUNT_SID}:${process.env.TWILIO_AUTH_TOKEN}`).toString('base64');
        const formBody = new URLSearchParams({
          To: sosData.emergencyContact,
          From: process.env.TWILIO_PHONE_NUMBER,
          Body: alertMessage
        });
        const twilioRes = await fetch(`https://api.twilio.com/2010-04-01/Accounts/${process.env.TWILIO_ACCOUNT_SID}/Messages.json`, {
          method: 'POST',
          headers: {
            'Authorization': `Basic ${auth}`,
            'Content-Type': 'application/x-www-form-urlencoded'
          },
          body: formBody
        });
        const json = await twilioRes.json();
        if (twilioRes.ok) {
          console.log(`[SMS Gateway] Twilio SMS dispatched! SID: ${json.sid}`);
          smsSuccess = true;
          providerName = 'Twilio SMS';
        } else {
          console.warn(`[SMS Gateway] Twilio error:`, json.message);
        }
      } catch (err) {
        console.error(`[SMS Gateway] Twilio dispatch failed:`, err.message);
      }
    }

    // 2. Live Telegram Bot Notification (Zero cost, instant phone ping)
    if (process.env.TELEGRAM_BOT_TOKEN && process.env.TELEGRAM_CHAT_ID) {
      try {
        const tgRes = await fetch(`https://api.telegram.org/bot${process.env.TELEGRAM_BOT_TOKEN}/sendMessage`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            chat_id: process.env.TELEGRAM_CHAT_ID,
            text: alertMessage
          })
        });
        if (tgRes.ok) {
          console.log(`[Telegram Gateway] Live Telegram SOS message dispatched!`);
          smsSuccess = true;
          providerName = providerName === 'Simulated Cloud Gateway' ? 'Telegram Bot' : `${providerName} + Telegram`;
        }
      } catch (err) {
        console.error(`[Telegram Gateway] Dispatch failed:`, err.message);
      }
    }

    if (!smsSuccess) {
      console.log(`[SMS Gateway] Broadcast logged to simulated emergency channel. (Configure Twilio / Telegram env vars for real-world cellular delivery).`);
    }

    // Acknowledge back to all connected dashboards
    io.emit('sos-dispatched-confirmation', {
      success: true,
      provider: providerName,
      lat: sosData.lat,
      lon: sosData.lon,
      mapsUrl: sosData.mapsUrl,
      emergencyContact: sosData.emergencyContact,
      timestamp: Date.now()
    });
  });
});

autoConnectArduino();

server.listen(PORT, () => {
  console.log(`=======================================================`);
  console.log(`  🚀 AegisHelm Stitch IoT Full-Stack Platform Active!`);
  console.log(`  Live Cockpit: http://localhost:${PORT}`);
  console.log(`  Trip Logs:    http://localhost:${PORT}/trip-logs`);
  console.log(`=======================================================`);
});
