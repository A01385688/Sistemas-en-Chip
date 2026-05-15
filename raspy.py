import json
import os
import queue
import threading
import time
from csv import writer

import FreeSimpleGUI as sg
import matplotlib.pyplot as plt
import paho.mqtt.client as mqtt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

BROKER_IP   = "192.168.137.177"   
BROKER_PORT = 1883
TOPIC       = "tractor/datos"     
CLIENT_ID   = "Python_Monitor"
ARCHIVO = "datos_tractor.csv"
TITULOS = ["Timestamp", "Velocidad Motor (RPM)", "Velocidad Vehiculo (km/h)", "Marcha"]

if not os.path.exists(ARCHIVO):
    with open(ARCHIVO, mode="w", newline="") as f:
        writer(f).writerow(TITULOS)

cola: "queue.Queue[dict]" = queue.Queue()
_conectado = threading.Event()

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[MQTT] Conectado al broker {BROKER_IP}:{BROKER_PORT}")
        client.subscribe(TOPIC)
        print(f"[MQTT] Suscrito a '{TOPIC}'")
        _conectado.set()
    else:
        print(f"[MQTT] Error de conexión rc={rc}")
        _conectado.clear()

def on_disconnect(client, userdata, rc):
    print(f"[MQTT] Desconectado (rc={rc})")
    _conectado.clear()

def on_message(client, userdata, msg):
    """
    Recibe: {"rpm":2400,"speed":60,"gear":2}
    Encola el dict ya parseado para que el hilo GUI lo consuma.
    """
    try:
        datos = json.loads(msg.payload.decode("utf-8"))
        rpm   = float(datos["rpm"])
        speed = float(datos["speed"])
        gear  = float(datos["gear"])
        cola.put({
            "rpm":       rpm,
            "speed":     speed,
            "gear":      gear,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "t_seg":     round(time.time() - t_inicio, 1),
        })
    except (json.JSONDecodeError, KeyError, ValueError) as e:
        print(f"[MQTT] Mensaje inválido: {msg.payload} → {e}")

t_inicio = time.time()

mqtt_client = mqtt.Client(client_id=CLIENT_ID)
mqtt_client.on_connect    = on_connect
mqtt_client.on_disconnect = on_disconnect
mqtt_client.on_message    = on_message

def _hilo_mqtt():
    while True:
        try:
            mqtt_client.connect(BROKER_IP, BROKER_PORT, keepalive=60)
            mqtt_client.loop_forever()          # bloquea hasta disconnect()
        except Exception as e:
            print(f"[MQTT] No se pudo conectar: {e} — reintentando en 5 s")
            time.sleep(5)

threading.Thread(target=_hilo_mqtt, daemon=True).start()

sg.theme("DarkBlack1")

COL_RPM   = "#7FFF00"
COL_SPEED = "#00BFFF"
COL_GEAR  = "#FF7F50"
FONT_BIG  = ("Arial", 22, "bold")
FONT_LBL  = ("Arial", 10)

layout = [
    # ── Título ──────────────────────────────────────────────────────────────
    [sg.Text("Monitor Tractor — Tiempo Real",
             font=("Arial", 17, "bold"), text_color="#FFD700", pad=(10, 8))],

    # ── Estado MQTT ─────────────────────────────────────────────────────────
    [sg.Text(f"Broker: {BROKER_IP}:{BROKER_PORT}  |  Topic: {TOPIC}",
             font=("Arial", 9), text_color="gray"),
     sg.Push(),
     sg.Text("●", key="-LED-", font=("Arial", 16), text_color="orange"),
     sg.Text("Conectando…", key="-ESTADO-", font=("Arial", 10), text_color="orange",
             size=(14, 1))],

    [sg.HorizontalSeparator(pad=(0, 6))],

    # ── Valores actuales ────────────────────────────────────────────────────
    [
        sg.Frame("Velocidad Motor", [
            [sg.Text("---", key="-RPM-", font=FONT_BIG,
                     text_color=COL_RPM, size=(9, 1), justification="center")],
            [sg.Text("RPM", font=FONT_LBL, text_color="gray",
                     justification="center", expand_x=True)],
        ], title_color=COL_RPM, pad=(8, 4)),

        sg.Frame("Velocidad Vehiculo", [
            [sg.Text("---", key="-SPEED-", font=FONT_BIG,
                     text_color=COL_SPEED, size=(9, 1), justification="center")],
            [sg.Text("km/h", font=FONT_LBL, text_color="gray",
                     justification="center", expand_x=True)],
        ], title_color=COL_SPEED, pad=(8, 4)),

        sg.Frame("Marcha", [
            [sg.Text("---", key="-GEAR-", font=FONT_BIG,
                     text_color=COL_GEAR, size=(9, 1), justification="center")],
            [sg.Text("Relacion", font=FONT_LBL, text_color="gray",
                     justification="center", expand_x=True)],
        ], title_color=COL_GEAR, pad=(8, 4)),
    ],

    # ── Contador ────────────────────────────────────────────────────────────
    [sg.Text("Muestras recibidas:", font=("Arial", 10)),
     sg.Text("0", key="-N-", font=("Arial", 12, "bold"), text_color="white"),
     sg.Push(),
     sg.Text("Ultimo mensaje:", font=("Arial", 10)),
     sg.Text("---", key="-TS-", font=("Arial", 10), text_color="gray")],

    [sg.HorizontalSeparator(pad=(0, 6))],

    # ── Graficas ────────────────────────────────────────────────────────────
    [sg.Canvas(key="-C1-", size=(430, 250)),
     sg.Canvas(key="-C2-", size=(430, 250))],

    [sg.HorizontalSeparator(pad=(0, 6))],
    [sg.Button("Limpiar", button_color=("white", "#333333")),
     sg.Push(),
     sg.Button("Cerrar", button_color=("white", "#8B0000"))],
]

window = sg.Window("Monitor Tractor MQTT", layout,
                   finalize=True, background_color="#1a1a1a")


BG_FIG = "#1c1c1c"
BG_AX  = "#222222"

def _estilizar_ax(fig, ax, titulo, ylabel):
    fig.patch.set_facecolor(BG_FIG)
    ax.set_facecolor(BG_AX)
    ax.set_title(titulo, color="white", fontsize=10, pad=5)
    ax.set_xlabel("Tiempo (s)", color="white", fontsize=8)
    ax.set_ylabel(ylabel,       color="white", fontsize=8)
    ax.tick_params(colors="white", labelsize=7)
    for sp in ax.spines.values():
        sp.set_color("#444444")
    ax.grid(color="#333333", linestyle="--", linewidth=0.5)

# Grafica 1 — RPM
fig1, ax1 = plt.subplots(figsize=(4.3, 2.6))
_estilizar_ax(fig1, ax1, "Velocidad del Motor", "RPM")
(ln_rpm,) = ax1.plot([], [], color=COL_RPM, lw=1.6, label="RPM")
ax1.legend(facecolor="#2c2c2c", labelcolor="white", fontsize=8)

# Grafica 2 — Speed + Gear
fig2, ax2 = plt.subplots(figsize=(4.3, 2.6))
_estilizar_ax(fig2, ax2, "Velocidad Vehiculo  y  Marcha", "Valor")
(ln_spd,)  = ax2.plot([], [], color=COL_SPEED, lw=1.6, label="km/h")
(ln_gear,) = ax2.plot([], [], color=COL_GEAR,  lw=1.6, label="Marcha",
                      linestyle="--")
ax2.legend(facecolor="#2c2c2c", labelcolor="white", fontsize=8)

# Incrustar canvases
cv1 = FigureCanvasTkAgg(fig1, master=window["-C1-"].TKCanvas)
cv1.get_tk_widget().pack(fill="both", expand=True)
cv2 = FigureCanvasTkAgg(fig2, master=window["-C2-"].TKCanvas)
cv2.get_tk_widget().pack(fill="both", expand=True)

# Historial
h_t, h_rpm, h_spd, h_gear = [], [], [], []

def _redibujar():
    ln_rpm.set_data(h_t, h_rpm)
    ax1.relim(); ax1.autoscale_view()
    cv1.draw_idle()

    ln_spd.set_data(h_t, h_spd)
    ln_gear.set_data(h_t, h_gear)
    ax2.relim(); ax2.autoscale_view()
    cv2.draw_idle()

def _limpiar():
    for lst in (h_t, h_rpm, h_spd, h_gear):
        lst.clear()
    _redibujar()


n_muestras     = 0
mqtt_ok_previo = False

while True:
    # 50 ms de timeout — ~20 ciclos/s, sin bloquear la GUI
    event, _ = window.read(timeout=50)

    if event in (sg.WIN_CLOSED, "Cerrar"):
        break
    if event == "Limpiar":
        _limpiar()
        n_muestras = 0
        window["-N-"].update("0")

    # ── Indicador de conexion ──────────────────────────────────────────────
    mqtt_ok = mqtt_client.is_connected()
    if mqtt_ok != mqtt_ok_previo:
        if mqtt_ok:
            window["-ESTADO-"].update("Conectado  v", text_color="#7FFF00")
            window["-LED-"].update(text_color="#7FFF00")
        else:
            window["-ESTADO-"].update("Desconectado", text_color="orange")
            window["-LED-"].update(text_color="orange")
        mqtt_ok_previo = mqtt_ok

    # ── Consumir TODOS los mensajes pendientes en este ciclo ───────────────
    hay_nuevos = False
    while not cola.empty():
        d = cola.get_nowait()

        # Actualizar etiquetas
        window["-RPM-"].update(f"{d['rpm']:.0f}")
        window["-SPEED-"].update(f"{d['speed']:.1f}")
        window["-GEAR-"].update(f"{int(d['gear'])}")
        window["-TS-"].update(d["timestamp"])

        # Historial para graficas
        h_t.append(d["t_seg"])
        h_rpm.append(d["rpm"])
        h_spd.append(d["speed"])
        h_gear.append(d["gear"])

        # Guardar CSV
        with open(ARCHIVO, mode="a", newline="") as f:
            writer(f).writerow(
                [d["timestamp"], d["rpm"], d["speed"], int(d["gear"])]
            )

        n_muestras += 1
        window["-N-"].update(str(n_muestras))
        hay_nuevos = True

    # Redibujar solo si llegaron datos nuevos en este ciclo
    if hay_nuevos:
        _redibujar()

# =============================================================================
window.close()
mqtt_client.disconnect()
print("[INFO] Aplicacion cerrada.")