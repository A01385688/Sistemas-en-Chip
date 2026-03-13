import os
import webbrowser
import datetime
import time

# Lista de tareas con prioridad e importancia
tareas = [
    {"nombre": "Ensayo de literatura", "archivo": "ensayo.docx", "hora": "23:30", "importancia": "alta"},
    {"nombre": "Reporte financiero", "archivo": "reporte.xlsx", "hora": "09:00", "importancia": "media"},
    {"nombre": "Cita con doctor", "archivo": None, "hora": "15:00", "importancia": "alta"}
]

def abrir_archivo(archivo):
    if archivo:
        os.startfile(archivo)  # En Windows abre el archivo con el programa predeterminado
    else:
        print("No hay archivo asociado a esta tarea.")

def notificar(tarea):
    print(f"Recordatorio: {tarea['nombre']} (Importancia: {tarea['importancia']})")
    abrir_archivo(tarea["archivo"])

def consultar_internet(tarea):
    if "doctor" in tarea["nombre"].lower():
        webbrowser.open("https://www.doctoralia.com.mx")  # Ejemplo de dashboard médico
        print("Se abrió la página de doctores disponibles.")

# Simulación de agenda
while True:
    hora_actual = datetime.datetime.now().strftime("%H:%M")
    for tarea in tareas:
        if tarea["hora"] == hora_actual:
            notificar(tarea)
            consultar_internet(tarea)
    time.sleep(60)  # Revisa cada minuto
