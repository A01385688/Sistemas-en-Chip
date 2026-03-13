#!/bin/bash
DB_PATH="tasks.db"

while [true]
do
	TAREA=$(sqlite3 $DB_PATH "SELECT name, file_path, category FROM tasks WHERE status='pending' LIMIT 1;")
	if["$TAREA" !=""]
	then
		NAME=$(echo "$TAREA" | cut -d '|' -f 1)
        	FILE=$(echo "$TAREA" | cut -d '|' -f 2)
        	CAT=$(echo "$TAREA" | cut -d '|' -f 3)
		notify-send "Alerta de tarea" "Iniciando tarea: $NAME"
		if [ "$CAT" == "Salud" ]
		then
			notify-send "Explorando internet" "Buscando doctores disponibles..."
			INFO_WEB=$(curl -s "https://api.disponibilidad.com/doctores" | head -n 5)
			echo "Datos recibidos: $INFO_WEB" > reporte_cita.txt
			xdg-open reporte_cita.txt &
		elif [ "$CAT" == "VHDL" ]
		then
            		code "$FILE" &
		fi
	sleep 30
done
