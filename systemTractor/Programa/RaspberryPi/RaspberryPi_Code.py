Main.py
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
import json
import queue
import threading
import time
from datetime import datetime, timezone
import paho.mqtt.client as mqtt
from http.server import BaseHTTPRequestHandler, HTTPServer
import os
import sys
import base64

ruta_script = os.path.dirname(os.path.abspath(__file__))
if ruta_script not in sys.path:
    sys.path.append(ruta_script)

try:
    from vision_tractor import iniciar_vision
    VISION_DISPONIBLE = True
    print("[VISION] Modulo de vision cargado OK")
except ImportError:
    VISION_DISPONIBLE = False
    print("[VISION] vision_tractor.py no encontrado - sistema corre sin vision")

BROKER_IP = "10.43.146.251"
BROKER_PORT = 1883
TOPIC = "tractor/datos"
TOPIC_CMD = "tractor/control"
CLIENT_ID = "Python_Monitor"

TOKEN = "f__dH809NGhgUUFjQF3JOwy-Ma46dk_78B6xX5UUctnrxru3uFNw-XrCEWiEOLfCdWr8LYLRSCGaNvAX-eEctQ=="
ORG = "FacultadIng"
BUCKET = "telemetria_tractor"
URL = "http://localhost:8086"

id_dispositivo = "STM32_01"
operador_id = "operador"

cola: "queue.Queue[dict]" = queue.Queue()
_conectado = threading.Event()
t_inicio = time.time()

_ultimo_stm = {
    "m": 0, "a": 0, "b": 0,
    "r": 0, "v": 0.0, "g": 1,
    "pf": 0, "pt": 0,
    "s1": 90, "s2": 0, "s3": 0,
}
_lock_stm = threading.Lock()

_desviacion_px = 0.0
_lock_desv = threading.Lock()

influx_client = InfluxDBClient(url=URL, token=TOKEN, org=ORG)
write_api = influx_client.write_api(write_options=SYNCHRONOUS)

def escribir_influx(datos: dict) -> None:
    try:
        if datos.get("tipo") == "vision":
            punto = (
                Point("vision_tractor")
                .tag("id_dispositivo", id_dispositivo)
                .tag("operador_id", operador_id)
                .field("desviacion_px", float(datos["desviacion_px"]))
                .field("desviacion_norm", float(datos["desviacion_norm"]))
                .field("angulo_giro", float(datos["angulo_giro"]))
                .field("confianza", float(datos["confianza"]))
                .field("lineas", int(datos["lineas"]))
                .field("hilera_detectada", int(datos.get("hilera_detectada", 0)))
            )
            write_api.write(bucket=BUCKET, org=ORG, record=punto)
            print(f"[InfluxDB] Vision OK | Desv={datos['desviacion_px']:+.0f}px | Ang={datos['angulo_giro']:+.1f} | Conf={datos['confianza']*100:.0f}%")
            return

        punto = (
            Point("telemetria_tractor")
            .tag("id_dispositivo", id_dispositivo)
            .tag("operador_id", operador_id)
            .field("rpm", float(datos["rpm"]))
            .field("velocidad", float(datos["speed"]))
            .field("marcha", int(datos["gear"]))
        )
        write_api.write(bucket=BUCKET, org=ORG, record=punto)
        print(f"[InfluxDB] OK | RPM={datos['rpm']:.0f} | Vel={datos['speed']:.1f} km/h | Marcha={int(datos['gear'])}")

    except Exception as e:
        print(f"[InfluxDB] Error al escribir: {e}")

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"[MQTT] Conectado al broker {BROKER_IP}:{BROKER_PORT}")
        client.subscribe(TOPIC)
        print(f"[MQTT] Suscrito a '{TOPIC}'")
        _conectado.set()
    else:
        print(f"[MQTT] Error de conexion rc={rc}")
        _conectado.clear()

def on_disconnect(client, userdata, rc, properties=None):
    print(f"[MQTT] Desconectado (rc={rc})")
    _conectado.clear()

def on_message(client, userdata, msg):
    try:
        datos = json.loads(msg.payload.decode("utf-8"))

        if "r" in datos:
            rpm = float(datos.get("r", 0))
            speed = float(datos.get("v", 0))
            gear = float(datos.get("g", 1))

            with _lock_stm:
                _ultimo_stm["m"] = int(datos.get("m", _ultimo_stm["m"]))
                _ultimo_stm["a"] = int(datos.get("a", _ultimo_stm["a"]))
                _ultimo_stm["b"] = int(datos.get("b", _ultimo_stm["b"]))
                _ultimo_stm["r"] = int(rpm)
                _ultimo_stm["v"] = speed
                _ultimo_stm["g"] = int(gear)
                _ultimo_stm["pf"] = int(datos.get("pf", _ultimo_stm["pf"]))
                _ultimo_stm["pt"] = int(datos.get("pt", _ultimo_stm["pt"]))
                _ultimo_stm["s1"] = int(datos.get("s1", _ultimo_stm["s1"]))
                _ultimo_stm["s2"] = int(datos.get("s2", _ultimo_stm["s2"]))
                _ultimo_stm["s3"] = int(datos.get("s3", _ultimo_stm["s3"]))

            print(f"[STM32] m={_ultimo_stm['m']} a={_ultimo_stm['a']} b={_ultimo_stm['b']} r={int(rpm)} v={speed:.1f} g={int(gear)} pf={_ultimo_stm['pf']} pt={_ultimo_stm['pt']} s1={_ultimo_stm['s1']} s2={_ultimo_stm['s2']} s3={_ultimo_stm['s3']}")
        else:
            rpm = float(datos["rpm"])
            speed = float(datos["speed"])
            gear = float(datos["gear"])

        cola.put({
            "rpm": rpm,
            "speed": speed,
            "gear": gear,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "t_seg": round(time.time() - t_inicio, 1),
        })

    except (json.JSONDecodeError, KeyError, ValueError) as e:
        print(f"[MQTT] Mensaje invalido: {msg.payload} -> {e}")

try:
    mqtt_client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=CLIENT_ID,
    )
except AttributeError:
    mqtt_client = mqtt.Client(client_id=CLIENT_ID)

mqtt_client.on_connect = on_connect
mqtt_client.on_disconnect = on_disconnect
mqtt_client.on_message = on_message

def _hilo_mqtt():
    while True:
        try:
            mqtt_client.connect(BROKER_IP, BROKER_PORT, keepalive=60)
            mqtt_client.loop_forever()
        except Exception as e:
            print(f"[MQTT] No se pudo conectar: {e} - reintentando en 5 s")
            time.sleep(5)

threading.Thread(target=_hilo_mqtt, daemon=True).start()

def _desviacion_a_servo(desv_px: float, rango_px: float = 160.0, centro: int = 90, margen: int = 90) -> int:
    norm = max(-1.0, min(1.0, desv_px / rango_px))
    angulo = int(centro + norm * margen)
    return max(0, min(180, angulo))

def _publicar(payload: dict) -> bool:
    if not _conectado.is_set():
        print(f"[CONTROL] No conectado, no se publicó: {payload}")
        return False
    data = json.dumps({**payload, "ts": datetime.now(timezone.utc).isoformat()})
    result = mqtt_client.publish(TOPIC_CMD, data, qos=1)
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"[CONTROL] Publicado: {payload}")
        return True
    print(f"[CONTROL] Error al publicar (rc={result.rc})")
    return False

def enviar_comando(accion: str, val: int = -1) -> bool:
    with _lock_desv:
        desv = _desviacion_px

    s1_val = _desviacion_a_servo(desv)

    if accion == "frenar":
        ok = _publicar({"cmd": "mode_manual"})
        ok &= _publicar({"cmd": "frenar", "val": 1})
        ok &= _publicar({"cmd": "servo1", "val": s1_val})
        ok &= _publicar({"cmd": "servo2", "val": 0})
        ok &= _publicar({"cmd": "servo3", "val": 0})
        return ok

    if accion == "acelerar":
        accel = val if (0 <= val <= 100) else 80
        ok = _publicar({"cmd": "mode_instrucciones"})
        ok &= _publicar({"cmd": "acelerar", "val": accel})
        ok &= _publicar({"cmd": "frenar", "val": 0})
        ok &= _publicar({"cmd": "servo1", "val": s1_val})
        ok &= _publicar({"cmd": "servo2", "val": 0})
        ok &= _publicar({"cmd": "servo3", "val": 0})
        return ok

    if accion == "neutro":
        ok = _publicar({"cmd": "neutro"})
        ok &= _publicar({"cmd": "servo1", "val": s1_val})
        ok &= _publicar({"cmd": "servo2", "val": 0})
        ok &= _publicar({"cmd": "servo3", "val": 0})
        return ok

    return _publicar({"cmd": accion})

API_PORT = 8765

class _HandlerControl(BaseHTTPRequestHandler):

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_POST(self):
        try:
            length = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(length) or b'{}')
            cmd = body.get("cmd", "neutro")
            val = int(body.get("val", -1))
            ok = enviar_comando(cmd, val)

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "Content-Type")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": ok, "cmd": cmd}).encode())

        except Exception as e:
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps({"error": str(e)}).encode())

    def log_message(self, *args):
        pass

def _hilo_api():
    servidor = HTTPServer(("0.0.0.0", API_PORT), _HandlerControl)
    print(f"[API] Control remoto escuchando en http://localhost:{API_PORT}")
    servidor.serve_forever()

threading.Thread(target=_hilo_api, daemon=True, name="hilo-api").start()

def _hilo_servo_vision():
    while True:
        time.sleep(0.2)
        with _lock_stm:
            modo = _ultimo_stm["m"]
        if modo == 1:
            with _lock_desv:
                desv = _desviacion_px
            s1 = _desviacion_a_servo(desv)
            _publicar({"cmd": "servo1", "val": s1})
            _publicar({"cmd": "servo2", "val": 0})
            _publicar({"cmd": "servo3", "val": 0})

threading.Thread(target=_hilo_servo_vision, daemon=True, name="hilo-servo").start()

def main():
    print("=" * 60)
    print("Iniciando sistema")
    print(f"  Bucket  : {BUCKET}")
    print(f"  Measure : telemetria_tractor + vision_tractor")
    print(f"  Tags    : id_dispositivo={id_dispositivo} | operador_id={operador_id}")
    print(f"  API     : http://localhost:{API_PORT}")
    print("=" * 60)

    if VISION_DISPONIBLE:
        def _cola_vision_con_desv(cola_ext):
            class _QueueProxy:
                def put(self_, item, **kwargs):
                    if item.get("tipo") == "vision":
                        global _desviacion_px
                        with _lock_desv:
                            _desviacion_px = float(item.get("desviacion_px", 0.0))
                    cola_ext.put(item, **kwargs)
            return _QueueProxy()

        iniciar_vision(
            cola_externa = _cola_vision_con_desv(cola),
            publicar_mqtt = True,
            mostrar_preview = False,
            write_api = write_api,
            bucket = BUCKET,
            org = ORG,
            id_dispositivo = id_dispositivo,
        )
        print("[MAIN] Modulo de vision arrancado")

    if not _conectado.wait(timeout=30):
        print("[MAIN] Sin conexion MQTT tras 30 s - continuando de todas formas")

    try:
        while True:
            try:
                datos = cola.get(timeout=1.0)
                escribir_influx(datos)
            except queue.Empty:
                pass
    except KeyboardInterrupt:
        print("\n[MAIN] Cierre solicitado por el usuario.")
    finally:
        print("[MAIN] Cerrando conexiones...")
        try:
            mqtt_client.disconnect()
        except Exception:
            pass
        write_api.close()
        influx_client.close()
        print("[MAIN] Sistema detenido correctamente.")

if __name__ == "__main__":
    main()



vision_tractor.py
import cv2
import numpy as np
import time
import threading
import queue
import json
import base64
from dataclasses import dataclass, asdict
from typing import Optional, Tuple
import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point, WritePrecision
from datetime import datetime, timezone

ESP32_CAM_IP = "10.43.146.120"
ESP32_STREAM = f"http://{ESP32_CAM_IP}:81/stream"

BROKER_IP = "10.43.146.251"
BROKER_PORT = 1883
TOPIC_VISION = "tractor/vision"
TOPIC_CMD = "tractor/control"

RESIZE_WIDTH = 320
RESIZE_HEIGHT = 240
ROI_Y_START = 0.45
BLUR_KERNEL = (5, 5)
CANNY_LOW = 50
CANNY_HIGH = 150
HOUGH_RHO = 1
HOUGH_THETA = np.pi/180
HOUGH_THRESHOLD = 40
HOUGH_MIN_LENGTH = 50
HOUGH_MAX_GAP = 30

PUBLISH_INTERVAL = 0.2
SHOW_PREVIEW = True
PREVIEW_SCALE = 1.5
KEYFRAME_JPEG_QUALITY = 70  
ESTADOS_KEYFRAME = {"SIN_LINEAS", "SIN_PUNTO_FUGA"}

@dataclass
class VisionResult:
    timestamp: str
    desviacion_px: float
    desviacion_norm: float
    angulo_giro: float
    lineas_detectadas: int
    confianza: float
    estado: str

class DetectorHileras:
    def __init__(self, ancho: int = RESIZE_WIDTH, alto: int = RESIZE_HEIGHT):
        self.ancho = ancho
        self.alto = alto
        self.cx = ancho // 2
        self._historial: list[float] = []
        self._max_historial = 5

    @staticmethod
    def _angulo_linea(x1: int, y1: int, x2: int, y2: int) -> float:
        dx = x2 - x1
        dy = y2 - y1
        if dy == 0:
            return 90.0
        return float(np.degrees(np.arctan2(abs(dx), abs(dy))))

    def _filtrar_lineas(self, lineas: np.ndarray) -> Tuple[list, list]:
        izq, der = [], []
        for linea in lineas:
            x1, y1, x2, y2 = linea[0]
            ang = self._angulo_linea(x1, y1, x2, y2)
            if ang > 65:
                continue
            cx_linea = (x1 + x2) / 2
            pendiente = (y2 - y1) / (x2 - x1 + 1e-6)
            if pendiente < 0 and cx_linea < self.cx:
                izq.append(linea[0])
            elif pendiente > 0 and cx_linea >= self.cx:
                der.append(linea[0])
        return izq, der

    @staticmethod
    def _linea_promedio(lineas: list, alto: int) -> Optional[Tuple[int, int, int, int]]:
        if not lineas:
            return None
        xs, ys = [], []
        for x1, y1, x2, y2 in lineas:
            xs.extend([x1, x2])
            ys.extend([y1, y2])
        if len(xs) < 2:
            return None
        coef = np.polyfit(xs, ys, 1)
        m, b = coef
        y_bot = alto
        y_top = int(alto * 0.5)
        x_bot = int((y_bot - b) / (m + 1e-9))
        x_top = int((y_top - b) / (m + 1e-9))
        return (x_bot, y_bot, x_top, y_top)

    def _punto_fuga(self, linea_izq: Optional[Tuple], linea_der: Optional[Tuple]) -> Optional[float]:
        if linea_izq and linea_der:
            (xi1, yi1, xi2, yi2) = linea_izq
            (xd1, yd1, xd2, yd2) = linea_der
            mi = (yi2 - yi1) / (xi2 - xi1 + 1e-9)
            md = (yd2 - yd1) / (xd2 - xd1 + 1e-9)
            bi = yi1 - mi * xi1
            bd = yd1 - md * xd1
            if abs(mi - md) < 1e-6:
                return None
            x_f = (bd - bi) / (mi - md)
            return float(x_f)
        elif linea_izq:
            return float(linea_izq[2])
        elif linea_der:
            return float(linea_der[2])
        return None

    def procesar(self, frame: np.ndarray) -> Tuple[VisionResult, np.ndarray]:
        ts = time.strftime("%Y-%m-%d %H:%M:%S")
        frame = cv2.resize(frame, (self.ancho, self.alto))
        h, w = frame.shape[:2]
        frame_limpio = frame.copy()
        roi_y = int(h * ROI_Y_START)
        roi = frame[roi_y:h, 0:w]
        gris = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
        blur = cv2.GaussianBlur(gris, BLUR_KERNEL, 0)
        canny = cv2.Canny(blur, CANNY_LOW, CANNY_HIGH)

        lineas = cv2.HoughLinesP(
            canny,
            rho=HOUGH_RHO,
            theta=HOUGH_THETA,
            threshold=HOUGH_THRESHOLD,
            minLineLength=HOUGH_MIN_LENGTH,
            maxLineGap=HOUGH_MAX_GAP,
        )

        debug = frame.copy()
        overlay_roi = debug[roi_y:h, 0:w]

        n_lineas = 0
        desv_px = 0.0
        angulo = 0.0
        confianza = 0.0
        estado = "SIN_LINEAS"

        if lineas is not None:
            n_lineas = len(lineas)
            izq_raw, der_raw = self._filtrar_lineas(lineas)

            for ln in lineas:
                x1, y1, x2, y2 = ln[0]
                cv2.line(overlay_roi, (x1, y1), (x2, y2), (0, 80, 0), 1)

            linea_izq = self._linea_promedio(izq_raw, roi.shape[0])
            linea_der = self._linea_promedio(der_raw, roi.shape[0])

            if linea_izq:
                cv2.line(overlay_roi, (linea_izq[0], linea_izq[1]), (linea_izq[2], linea_izq[3]), (255, 80, 0), 3)
            if linea_der:
                cv2.line(overlay_roi, (linea_der[0], linea_der[1]), (linea_der[2], linea_der[3]), (0, 80, 255), 3)

            x_fuga = self._punto_fuga(linea_izq, linea_der)
            if x_fuga is not None:
                pf_abs = (int(x_fuga), roi_y + int(roi.shape[0] * 0.5))
                cv2.circle(debug, pf_abs, 8, (0, 255, 255), -1)
                desv_px = float(x_fuga - self.cx)
                desv_norm = np.clip(desv_px / (self.cx + 1e-9), -1.0, 1.0)
                self._historial.append(float(desv_norm))
                if len(self._historial) > self._max_historial:
                    self._historial.pop(0)
                desv_norm_suav = float(np.mean(self._historial))
                angulo = float(np.clip(desv_norm_suav * 30.0, -30.0, 30.0))
                base = min(n_lineas / 10.0, 1.0)
                bonus = 0.2 if (linea_izq and linea_der) else 0.0
                confianza = float(np.clip(base + bonus, 0.0, 1.0))
                estado = "OK"
            else:
                estado = "SIN_PUNTO_FUGA"
                self._historial.clear()

        cv2.line(debug, (self.cx, 0), (self.cx, h), (255, 255, 255), 1, cv2.LINE_AA)
        x_actual = int(self.cx + desv_px)
        color_dev = (0, 255, 0) if abs(desv_px) < 20 else (0, 165, 255) if abs(desv_px) < 50 else (0, 0, 255)
        cv2.line(debug, (x_actual, roi_y), (x_actual, h), color_dev, 2)
        cv2.rectangle(debug, (0, roi_y), (w-1, h-1), (200, 200, 0), 1)

        _hud(debug, estado, desv_px, angulo, confianza, n_lineas)

        resultado = VisionResult(
            timestamp=ts,
            desviacion_px=round(desv_px, 1),
            desviacion_norm=round(float(np.mean(self._historial)) if self._historial else 0.0, 3),
            angulo_giro=round(angulo, 1),
            lineas_detectadas=n_lineas,
            confianza=round(confianza, 2),
            estado=estado,
        )
        return resultado, debug, frame_limpio

def _hud(img, estado, desv, angulo, conf, n_lin):
    h, w = img.shape[:2]
    fondo = img.copy()
    cv2.rectangle(fondo, (0, 0), (w, 80), (0, 0, 0), -1)
    cv2.addWeighted(fondo, 0.55, img, 0.45, 0, img)
    color_estado = (0, 220, 0) if estado == "OK" else (0, 165, 255)
    cv2.putText(img, f"Estado : {estado}", (8, 18), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color_estado, 1)
    cv2.putText(img, f"Desv. : {desv:+.0f} px", (8, 36), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    cv2.putText(img, f"Angulo : {angulo:+.1f} deg", (8, 54), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    cv2.putText(img, f"Conf : {conf*100:.0f}% Lin: {n_lin}", (8, 72), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

class CapturaESP32:
    def __init__(self, url: str):
        self.url = url
        self._frame: Optional[np.ndarray] = None
        self._lock = threading.Lock()
        self._vivo = False
        self._hilo = threading.Thread(target=self._bucle, daemon=True, name="hilo-cam")

    def iniciar(self):
        self._hilo.start()

    def ultimo_frame(self) -> Optional[np.ndarray]:
        with self._lock:
            return None if self._frame is None else self._frame.copy()

    @property
    def conectado(self) -> bool:
        return self._vivo

    def _bucle(self):
        while True:
            cap = cv2.VideoCapture(self.url)
            if not cap.isOpened():
                print(f"[CAM] No se pudo abrir {self.url} - reintentando en 3 s")
                self._vivo = False
                time.sleep(3)
                continue

            print(f"[CAM] Stream conectado: {self.url}")
            self._vivo = True

            while True:
                ok, frame = cap.read()
                if not ok or frame is None:
                    print("[CAM] Perdida de frame - reconectando")
                    self._vivo = False
                    break
                with self._lock:
                    self._frame = frame

            cap.release()
            time.sleep(2)

class PublicadorVision:
    def __init__(self):
        try:
            self._cli = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2, client_id="Python_Vision")
        except AttributeError:
            self._cli = mqtt.Client(client_id="Python_Vision")

        self._cli.on_connect = self._on_connect
        self._cli.on_disconnect = self._on_disconnect
        self._conectado = threading.Event()
        self._cli.connect_async(BROKER_IP, BROKER_PORT, keepalive=60)
        self._cli.loop_start()

    def _on_connect(self, *args):
        print("[VISION-MQTT] Conectado al broker")
        self._conectado.set()

    def _on_disconnect(self, *args):
        print("[VISION-MQTT] Desconectado")
        self._conectado.clear()

    def publicar(self, resultado: VisionResult):
        if not self._conectado.is_set():
            return
        payload = json.dumps(asdict(resultado))
        self._cli.publish(TOPIC_VISION, payload, qos=0)

    def cerrar(self):
        self._cli.loop_stop()
        self._cli.disconnect()

def iniciar_vision(cola_externa: Optional["queue.Queue"] = None, publicar_mqtt: bool = True, mostrar_preview: bool = SHOW_PREVIEW, write_api=None, bucket: str = "telemetria_tractor", org: str = "FacultadIng", id_dispositivo: str = "STM32_01"):
    hilo = threading.Thread(
        target=_bucle_vision,
        args=(cola_externa, publicar_mqtt, mostrar_preview, write_api, bucket, org, id_dispositivo),
        daemon=True,
        name="hilo-vision",
    )
    hilo.start()
    return hilo

def _guardar_keyframe(frame_debug: np.ndarray, estado: str, write_api, bucket: str, org: str, id_dispositivo: str) -> None:
    try:
        encode_params = [cv2.IMWRITE_JPEG_QUALITY, KEYFRAME_JPEG_QUALITY]
        _, buffer = cv2.imencode('.jpg', frame_debug, encode_params)
        img_b64 = base64.b64encode(buffer).decode('utf-8')

        punto = (
            Point("keyframes_tractor")
            .tag("id_dispositivo", id_dispositivo)
            .tag("motivo", estado)
            .field("imagen", img_b64)
            #.time(datetime.now(timezone.utc), WritePrecision.NANOSECONDS)
        )
        write_api.write(bucket=bucket, org=org, record=punto)
        print(f"[VISION] Keyframe guardado motivo: {estado}")
    except Exception as e:
        print(f"[VISION] Error guardando keyframe: {e}")

def _bucle_vision(cola_externa: Optional["queue.Queue"], publicar_mqtt: bool, mostrar_preview: bool, write_api, bucket: str, org: str, id_dispositivo: str):
    camara = CapturaESP32(ESP32_STREAM)
    detector = DetectorHileras()
    publicador = PublicadorVision() if publicar_mqtt else None
    ultimo_pub = 0.0
    ultimo_estado = "OK"

    camara.iniciar()
    print("[VISION] Modulo de deteccion de hileras iniciado")
    print(f"[VISION] Stream: {ESP32_STREAM}")

    while True:
        frame = camara.ultimo_frame()
        if frame is None:
            time.sleep(0.05)
            continue

        resultado, debug, limpio = detector.procesar(frame)

        if (write_api is not None and resultado.estado in ESTADOS_KEYFRAME and resultado.estado != ultimo_estado):
            # En lugar de mandarle "debug", le mandamos el frame "limpio" a InfluxDB
            _guardar_keyframe(limpio, resultado.estado, write_api, bucket, org, id_dispositivo)
        ultimo_estado = resultado.estado

        ahora = time.time()
        if ahora - ultimo_pub >= PUBLISH_INTERVAL:
            ultimo_pub = ahora

            if cola_externa is not None:
                cola_externa.put({
                    "tipo": "vision",
                    "desviacion_px": resultado.desviacion_px,
                    "desviacion_norm": resultado.desviacion_norm,
                    "angulo_giro": resultado.angulo_giro,
                    "confianza": resultado.confianza,
                    "hilera_detectada": 1 if resultado.confianza > 0.3 else 0,
                    "lineas": resultado.lineas_detectadas,
                    "timestamp": resultado.timestamp,
                    "t_seg": round(time.time(), 1),
                })

            if publicador:
                publicador.publicar(resultado)

            print(f"[VISION] {resultado.estado} | Desv={resultado.desviacion_px:+.0f}px | Ang={resultado.angulo_giro:+.1f} | Conf={resultado.confianza*100:.0f}%")

        if mostrar_preview:
            escala = PREVIEW_SCALE
            ancho_v = int(debug.shape[1] * scales) if 'scales' in locals() else int(debug.shape[1] * escala)
            alto_v = int(debug.shape[0] * escala)
            vista = cv2.resize(debug, (ancho_v, alto_v))
            cv2.imshow("Deteccion de Hileras Tractor", vista)
            tecla = cv2.waitKey(1) & 0xFF
            if tecla == ord('q'):
                print("[VISION] Preview cerrado por usuario")
                cv2.destroyAllWindows()
                break

        time.sleep(0.02)

if __name__ == "__main__":
    import sys
    fuente: any = ESP32_STREAM

    if len(sys.argv) > 1:
        arg = sys.argv[1]
        fuente = int(arg) if arg.isdigit() else arg

    print("=" * 55)
    print(" Deteccion de hileras modo standalone")
    print(f" Fuente : {fuente}")
    print(" Presiona Q para salir")
    print("=" * 55)

    detector = DetectorHileras()
    cap = cv2.VideoCapture(fuente)

    if not cap.isOpened():
        print(f"[ERROR] No se pudo abrir la fuente: {fuente}")
        sys.exit(1)

    fps_contador = 0
    fps_ts = time.time()

    while True:
        ok, frame = cap.read()
        if not ok:
            print("[INFO] Fin de la fuente de video.")
            break

        resultado, debug = detector.procesar(frame)
        fps_contador += 1
        if time.time() - fps_ts >= 1.0:
            fps = fps_contador
            fps_contador = 0
            fps_ts = time.time()
            cv2.putText(debug, f"FPS: {fps}", (debug.shape[1]-80, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180, 180, 180), 1)

        escala = PREVIEW_SCALE
        vista = cv2.resize(debug, (int(debug.shape[1]*escala), int(debug.shape[0]*escala)))
        cv2.imshow("Deteccion de Hileras", vista)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
    print("[INFO] Standalone finalizado.")