#!/bin/bash
DB_PATH="tasks.db"

while true
do
    # 1. Obtener la tarea
    TAREA=$(sqlite3 $DB_PATH "SELECT name, file_path, category FROM tasks WHERE status='pending' LIMIT 1;")

    # 2. Verificar si hay contenido (Espacios corregidos)
    if [ "$TAREA" != "" ]
    then
        # 3. Separar datos con 'cut'
        NAME=$(echo "$TAREA" | cut -d '|' -f 1)
        FILE=$(echo "$TAREA" | cut -d '|' -f 2)
        CAT=$(echo "$TAREA" | cut -d '|' -f 3)

        notify-send "Orquestador" "Iniciando: $NAME"

        # 4. Lógica por categoría
        if [ "$CAT" == "Salud" ]
        then
            notify-send "Internet" "Consultando especialistas..."
            
            # API REAL: Traemos 5 "doctores" y formateamos la salida con jq
            INFO_WEB=$(curl -s "https://jsonplaceholder.typicode.com/users?_limit=5" | jq -r '.[] | "Dr. \(.name) - Consultorio: \(.address.city)"')
            
            echo "--- LISTA DE ESPECIALISTAS DISPONIBLES ---" > reporte_cita.txt
            echo "Generado el: $(date)" >> reporte_cita.txt
            echo "----------------------------------------" >> reporte_cita.txt
            echo "$INFO_WEB" >> reporte_cita.txt
            
            xdg-open reporte_cita.txt &

        elif [ "$CAT" == "VHDL" ] # O puedes cambiar la categoría a "Python"
        then
            # --- SCAMPER: Sustituir (Thonny por VS Code) ---
            # Verificamos si el archivo es .py para que Thonny no tenga problemas
            if [[ "$FILE" == *.py ]]; then
                notify-send "Programación" "Abriendo $NAME en Thonny..."
                thonny "$FILE" &
            else
                # Si es un archivo de otro tipo (como VHDL), usamos un editor de texto
                notify-send "Sistema" "Abriendo archivo de texto..."
                mousepad "$FILE" & 
            fi

        # 5. ACTUALIZACIÓN CRÍTICA: Marcar como hecha
        sqlite3 $DB_PATH "UPDATE tasks SET status='done' WHERE name='$NAME';"
        
        notify-send "Sistema" "Entorno para $NAME preparado y ticket cerrado."
    fi

    sleep 30
done
