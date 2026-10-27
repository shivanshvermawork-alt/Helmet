import os
import sys
import time
import json
import threading
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import serial
import serial.tools.list_ports

PORT = 8000
WORKSPACE_DIR = os.path.dirname(os.path.abspath(__file__))
DASHBOARD_DIR = os.path.join(WORKSPACE_DIR, 'aegishelm_live_cockpit_dashboard')
TRIP_LOGS_DIR = os.path.join(WORKSPACE_DIR, 'aegishelm_trip_logs_blackbox_scorecard')

# Global state
clients = set()
clients_lock = threading.Lock()
pause_serial_until = 0

current_telemetry = {
    "helmet": 0,
    "alcohol": 0,
    "drowsy": 0,
    "crash": 0,
    "relay": 0,
    "speedKmh": 0,
    "lcd1": "SMART HELMET SYS",
    "lcd2": "RECEIVER READY..",
    "status": "CONNECTING"
}

serial_connection = None
serial_lock = threading.Lock()

def broadcast(event_data):
    msg = f"data: {json.dumps(event_data)}\n\n"
    with clients_lock:
        dead_clients = set()
        for client in clients:
            try:
                client.wfile.write(msg.encode('utf-8'))
                client.wfile.flush()
            except Exception:
                dead_clients.add(client)
        clients.difference_update(dead_clients)

def parse_serial_line(line):
    global current_telemetry
    line = line.strip()
    if not line:
        return

    print(f"[SERIAL] {line}")

    if '[SAFE]' in line:
        current_telemetry.update({
            "helmet": 1,
            "alcohol": 0,
            "drowsy": 0,
            "crash": 0,
            "relay": 1,
            "speedKmh": 42,
            "lcd1": "HELMET: SAFE OK ",
            "lcd2": "ENGINE: STARTED ",
            "status": "SAFE"
        })
        broadcast(current_telemetry)

    elif '[ALCOHOL]' in line:
        current_telemetry.update({
            "helmet": 1,
            "alcohol": 1,
            "drowsy": 0,
            "crash": 0,
            "relay": 0,
            "speedKmh": 0,
            "lcd1": "ALCOHOL DETECTED",
            "lcd2": "ENGINE: LOCKED! ",
            "status": "ALCOHOL"
        })
        broadcast(current_telemetry)

    elif '[NO_HELMET]' in line:
        current_telemetry.update({
            "helmet": 0,
            "alcohol": 0,
            "drowsy": 0,
            "crash": 0,
            "relay": 0,
            "speedKmh": 0,
            "lcd1": "WEAR HELMET &   ",
            "lcd2": "FASTEN STRAP!   ",
            "status": "NO_HELMET"
        })
        broadcast(current_telemetry)

    elif '[CRASH]' in line or '[ACCIDENT]' in line:
        current_telemetry.update({
            "helmet": 1,
            "alcohol": 0,
            "drowsy": 0,
            "crash": 1,
            "relay": 0,
            "speedKmh": 0,
            "lcd1": "CRASH DETECTED! ",
            "lcd2": "SOS DISPATCHING ",
            "status": "CRASH"
        })
        broadcast(current_telemetry)

    elif 'RF Signal Lost' in line:
        current_telemetry.update({
            "relay": 0,
            "speedKmh": 0,
            "lcd1": "SIGNAL LOST     ",
            "lcd2": "ENGINE STOPPED  ",
            "status": "TIMEOUT"
        })
        broadcast(current_telemetry)

def release_port_for_flash(seconds=35):
    global serial_connection, pause_serial_until
    with serial_lock:
        if serial_connection and serial_connection.is_open:
            try:
                serial_connection.close()
            except Exception:
                pass
            serial_connection = None
    pause_serial_until = time.time() + seconds
    print(f"[GATEWAY] ⚠️ COM port temporarily released for {seconds}s to allow Arduino IDE upload!")

def serial_worker():
    global serial_connection, pause_serial_until
    target_ports = ['COM4', 'COM7', 'COM5']

    while True:
        # Check if port is paused for firmware flashing
        if time.time() < pause_serial_until:
            time.sleep(1)
            continue

        if serial_connection is None:
            # Try finding Arduino port
            ports = [p.device for p in serial.tools.list_ports.comports()]
            found_port = None
            for tp in target_ports:
                if tp in ports:
                    found_port = tp
                    break

            if found_port:
                try:
                    print(f"[GATEWAY] Connecting to {found_port} at 9600 baud...")
                    ser = serial.Serial(found_port, 9600, timeout=1)
                    with serial_lock:
                        serial_connection = ser
                    print(f"[GATEWAY] Connected to {found_port}!")
                    current_telemetry["status"] = f"CONNECTED ({found_port})"
                    broadcast(current_telemetry)
                except Exception as e:
                    print(f"[GATEWAY] Could not open {found_port}: {e}")
                    time.sleep(2)
            else:
                time.sleep(2)
        else:
            try:
                line = serial_connection.readline().decode('utf-8', errors='ignore')
                if line:
                    parse_serial_line(line)
            except Exception as e:
                print(f"[GATEWAY] Serial disconnected: {e}")
                with serial_lock:
                    try:
                        serial_connection.close()
                    except Exception:
                        pass
                    serial_connection = None
                time.sleep(1)

class CockpitHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DASHBOARD_DIR, **kwargs)

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path == '/events':
            self.send_response(200)
            self.send_header('Content-Type', 'text/event-stream')
            self.send_header('Cache-Control', 'no-cache')
            self.send_header('Connection', 'keep-alive')
            self.end_headers()

            # Send current state on connection immediately
            init_msg = f"data: {json.dumps(current_telemetry)}\n\n"
            try:
                self.wfile.write(init_msg.encode('utf-8'))
                self.wfile.flush()
            except Exception:
                return

            with clients_lock:
                clients.add(self)

            try:
                while True:
                    time.sleep(20)
                    self.wfile.write(b": keepalive\n\n")
                    self.wfile.flush()
            except Exception:
                with clients_lock:
                    clients.discard(self)
            return

        elif self.path in ('/trip-logs', '/trip-logs/'):
            trip_logs_file = os.path.join(TRIP_LOGS_DIR, 'code.html')
            if os.path.exists(trip_logs_file):
                self.send_response(200)
                self.send_header('Content-Type', 'text/html; charset=utf-8')
                self.end_headers()
                with open(trip_logs_file, 'rb') as f:
                    self.wfile.write(f.read())
                return

        elif self.path in ('/', '/index.html'):
            self.path = '/code.html'

        return super().do_GET()

    def do_POST(self):
        if self.path == '/api/release-port':
            release_port_for_flash(35)
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"success": true, "released": 35}')
            return

        elif self.path == '/api/killswitch':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            print(f"[REMOTE] Killswitch command: {body}")
            with serial_lock:
                if serial_connection and serial_connection.is_open:
                    serial_connection.write(b"LOCK\n")
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"success": true, "action": "LOCK"}')
            return

        return super().do_POST()

if __name__ == '__main__':
    t = threading.Thread(target=serial_worker, daemon=True)
    t.start()

    server = ThreadingHTTPServer(('0.0.0.0', PORT), CockpitHandler)
    print("==================================================================")
    print(f"  AEGISHELM LOCALHOST WEB SERVER RUNNING ON http://localhost:{PORT}")
    print("==================================================================")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
        sys.exit(0)
