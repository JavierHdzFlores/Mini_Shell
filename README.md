# 🐚 Custom Linux Shell en C

Una shell interactiva para Linux desarrollada completamente desde cero en el lenguaje C. Este proyecto implementa un parseador de comandos propio e interactúa directamente con el kernel del sistema operativo mediante **System Calls**, manejando desde redes y hardware hasta la memoria y la comunicación entre procesos (IPC).

## 🚀 Características Principales

El proyecto cuenta con un sistema robusto de parseo de tokens (`strtok`) que permite identificar comandos, banderas y argumentos. Soporta los siguientes comandos nativos desarrollados a medida:

### 🌐 Redes e Información de Usuarios
* **`ip` / `mac`**: Recupera y formatea las direcciones IP y MAC de todas las tarjetas de red del sistema utilizando sockets y la llamada `ioctl` (`SIOCGIFCONF`, `SIOCGIFHWADDR`).
* **`who`**: Lee la estructura `utmp` del sistema para listar los usuarios actualmente conectados.

### 💻 Hardware y Memoria
* **`free`**: Calcula y muestra el estado actual de la memoria RAM (Total, Usada, Libre, Compartida, Buffer/Cache) y la memoria Swap utilizando `sysinfo()`.
* **`numerosdisp <ruta_dispositivo>`**: Funciona como un "detective de hardware". Utiliza la llamada `stat` para determinar si un archivo es un dispositivo de bloques o de caracteres, y extrae sus números **Major** y **Minor** con macros del sistema.

### 💬 Comunicación Inter-Procesos (IPC) y Difusión
* **`wall <mensaje>`**: Envía un mensaje de alerta a todas las terminales conectadas.
* **`mesg <usuario> <mensaje>`**: Envía un mensaje privado a la cola de mensajes de un usuario en específico utilizando `msgget` y `msgsnd`.
* **`leer <usuario>`**: Lee los mensajes en espera desde la cola IPC usando `msgrcv`, funcionando como un buzón local (chat) entre terminales.

### 📂 Gestión de Archivos
* **`find <ruta> <archivo>`**: Búsqueda recursiva de archivos dentro de los directorios del sistema.

## 🛠️ Requisitos del Sistema
* Sistema Operativo: Linux (Debian, Ubuntu, Arch, etc.)
* Compilador: `GCC`
* Librerías estándar de C, de red (`sys/socket.h`, `sys/ioctl.h`) y de IPC (`sys/ipc.h`, `sys/msg.h`).

## ⚙️ Compilación y Ejecución

1. Clona este repositorio en tu máquina local:
   ```bash
   git clone [https://github.com/tu-usuario/tu-repositorio.git](https://github.com/tu-usuario/tu-repositorio.git)
Accede al directorio del proyecto:
    Bash

    cd tu-repositorio

  Compila el código fuente usando GCC:
    
    Bash

    gcc shell3.c -o shell

  Ejecuta la shell:
  
    Bash

    ./shell

(Nota: Para probar el comando wall, se recomienda ejecutar la shell con privilegios de administrador: sudo ./shell)
🧠 Arquitectura Interna

El ciclo principal de la shell funciona a través de un bucle infinito que:

    Lee la entrada del usuario usando un buffer.

    Tokeniza la cadena para separar comando, posibles banderas (-) y concatena inteligentemente los argumentos para comandos que requieren oraciones completas (como mesg o wall).

    Compara el token principal y deriva la ejecución a funciones modulares que invocan directamente a la API de Linux.

👨‍💻 Autores

    Javier Hernandez Flores
    Elvia Marlen Hernandez Garcia

Desarrollado como proyecto para la materia de Sistemas Operativos.


***
