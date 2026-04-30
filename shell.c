#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <locale.h>
#include <utmp.h>

#include <sys/sysinfo.h>
#include <sys/msg.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define PATH_MAX 4096
#define RUTA 1024
#define MAX_CMD 400
#define MAX_HIST 512
#define DIR_PERMS (S_IRWXU | S_IRWXG | S_IRWXO)

// vareiables globales
char historial[MAX_HIST][MAX_CMD];
int posicion_actual = 0;
int total_comandos = 0;

// definimos la estructura para almacenar el color segun el tipo de archivo
typedef struct
{
    char color[20];
    char tipo[20];

} ColorArchivo;
// estructura para mensajes
struct msgbuf
{
    long mtype;
    char mtext[MAX_HIST];
};

// inicializar estructura de colores para los archivosç
ColorArchivo colores[5] = {
    {"\033[34m", "directory"},  // azul para directorios
    {"\033[32m", "regular"},    // verde para archivos regulares
    {"\033[33m", "executable"}, // amarillo para archivos ejecutables
    {"\033[35m", "link"},       // magenta para enlaces simbólicos
    {"\033[31m", "other"}       // rojo para otros tipos de archivos
};

void obtener_mac_tarjetas();                            // mac
void obtener_ips_tarjetas();                            // ip
void obtener_conectividad();                            // who
void obtener_date();                                    // date
void historial_comandos();                              // history
void pwd(char *r);                                      // pwd imprime la ruta actual
void cambiar_directorio(char *path);                    // cd cambia el directorio actual al especificado por path, si path es NULL muestra un error
void crear_directorio(char *path);                      // mkdir crea un nuevo directorio con el nombre especificado por path, si path es NULL muestra un error
void listar_directorio(char *path, char *bandera);      // ls lista el contenido del directorio especificado por path, si path es NULL lista el contenido del directorio actual
void mostrar_info_archivo(char *path);                  // stat
void mostrar_info_sistema_archivos(char *path);         // vfstat
void visualizar_contenido_archivo(char *path);          // cat
int verificar_archivo_reg_ordi(char *path);             // verificaer si es  un archivo regular o un directorio
void borrar_archivo(char *path);                        // unlink
void renombrar_archivo(char *old_path, char *new_path); // rename
void encontrar_archivo_normal(char *path, char *name);
int encontrar_archivo(char *path, char *name); // find busca recursivamente un archivo con el nombre especificado por name en el directorio especificado por path, si path es NULL busca en el directorio actual
void mostrar_info_sistem(char *bandera);       // uname muestra información del sistema operativo

void enviar_mensaje_wall(char *mensaje);
void mostrar_free();
void mostrar_numeros_dispositivo(char *path);
// void mostrar_vfstat();
void encontrar_archivo_recursivo(char *path, char *name);
void enviar_por_cola(char *nombre_usuario, char *texto);
void leer_buzon(char *mi_nombre);
void enviar_mesg(char *usuario, char *mensaje);

int main(void)
{
    // tendra la sentencia completa para usar strtok y separar el comando de los argumentos
    char sentencia[MAX_CMD];

    char comando[50], argumento_aux[50];
    // sera o un archivo o un directorio dependiendo del comando que se ejecute
    char argumento[200];
    // modificadores -a, -ls etc
    char bandera[50];
    // delimitador del espacio
    const char delimitador[] = " ";
    int centinela = 1, flag_sudo = 0;
    char *token;

    while (centinela == 1)
    {
        printf("\033[32m> \033[0m");

        if (fgets(sentencia, sizeof(sentencia), stdin) == NULL)
        {
            break;
        }
        // inicializamos comando,argumento,bandera
        comando[0] = '\0';
        argumento[0] = '\0';
        bandera[0] = '\0';
        argumento_aux[0] = '\0';

        sentencia[strcspn(sentencia, "\n")] = '\0'; // elimnar el salto de línea al final
        // historial de comando
        if (strlen(sentencia) > 0)
        {
            strncpy(historial[posicion_actual], sentencia, MAX_CMD - 1);
            historial[posicion_actual][MAX_CMD - 1] = '\0';     // Asegurar la terminación nula
            posicion_actual = (posicion_actual + 1) % MAX_HIST; // Avanzar la posición circularmente
            total_comandos++;
        }

        token = strtok(sentencia, delimitador); // separa el comando del argumento
        // verificamos si el primer token es sudo o un comando
        if (token == NULL)
        {
            continue; // si el comando esta vacio, se vuelve a pedir un comando
        }
        else
        {
            // si token encontro sudo activamos bandera  y avanzamos al comando
            if (strcmp(token, "sudo") == 0)
            {
                flag_sudo = 1;
                // avanzamos al siguiente tokem esperando el comabndo
                token = strtok(NULL, delimitador);
                // verificamos que token tenga el comando
                if (token == NULL)
                {
                    continue;
                }
                else
                {
                    strcpy(comando, token);
                }
            }
            else
            {
                strcpy(comando, token); // si el comando no esta vacio se guarda en la variable comando
            }
        }

        // mientras falte un valor de la sentencia, se seguira llamnado strtok
        token = strtok(NULL, delimitador); // separa el argumento del comando
        while (token != NULL)
        {
            // primer caso si se trata de badnera
            if (token[0] == '-')
            {
                strcpy(bandera, token); // si el token es una bandera se guarda en la variable bandera
            }
            else
            {
                // si argumento esta vacio se guarda en argumento, si no se guarda en argumento_aux
                if (argumento[0] == '\0')
                {
                    strcpy(argumento, token); // si el token no es una bandera se guarda en la variable argumento
                }
                else
                {

                    if (strcmp(comando, "wall") == 0)
                    {

                        strcat(argumento, " ");   // Agregamos espacio al mensaje
                        strcat(argumento, token); // Pegamos la siguiente palabra
                    }
                    else if (strcmp(comando, "mesg") == 0 || strcmp(comando, "wmesg") == 0)
                    {

                        if (argumento_aux[0] != '\0')
                        {
                            strcat(argumento_aux, " ");
                        }
                        strcat(argumento_aux, token);
                    }
                    else
                    {
                        strcpy(argumento_aux, token);
                    }
                }
            }

            token = strtok(NULL, delimitador);
        }

        // comparmos el comando ingresado con los comandos disponibles
        if (strcmp(comando, "exit") == 0)
        {
            centinela = 0;
        }
        else if (strcmp(comando, "stat") == 0)
        {
            if (argumento[0] == '\0')
            {
                fprintf(stderr, "Uso: stat <ruta> \n");
                fprintf(stderr, "Error al ejecutar stat\n");
            }
            else
            {
                mostrar_info_archivo(argumento);
            }
        }
        else if (strcmp(comando, "history") == 0)
        {
            historial_comandos();
        }
        else if (strcmp(comando, "cd") == 0)
        {
            if (argumento[0] == '\0')
            {
                cambiar_directorio(NULL);
            }
            else
            {
                cambiar_directorio(argumento);
            }
        }
        else if (strcmp(comando, "mkdir") == 0)
        {
            if (argumento[0] == '\0')
            {
                crear_directorio(NULL);
            }
            else
            {
                crear_directorio(argumento);
            }
        }
        else if (strcmp(comando, "ls") == 0)
        {
            if (argumento[0] == '\0')
            {
                listar_directorio(NULL, bandera);
            }
            else
            {
                listar_directorio(argumento, bandera);
            }
        }
        else if (strcmp(comando, "pwd") == 0)
        {
            pwd(argumento);
        }
        else if (strcmp(comando, "vfstat") == 0)
        {
            if (argumento[0] == '\0')
            {
                fprintf(stderr, "Uso: vfstat <ruta> \n");
                fprintf(stderr, "Error al ejecutar vfstat\n");
            }
            else
            {
                mostrar_info_sistema_archivos(argumento);
            }
        }
        else if (strcmp(comando, "rename") == 0)
        {
            // llamamos a strtok
            // char *segundo_argumento = strtok(NULL, " \n");

            if (argumento[0] == '\0' || argumento_aux[0] == '\0')
            {
                fprintf(stderr, "Uso: rename <nombre_viejo> <nombre_nuevo> \n");
                fprintf(stderr, "Error al ejecutar rename\n");
            }
            else
            {
                renombrar_archivo(argumento, argumento_aux);
            }
        }
        else if (strcmp(comando, "cat") == 0)
        {
            if (argumento[0] == '\0')
            {
                fprintf(stderr, "Uso: cat <directorio> \n");
                fprintf(stderr, "Error al ejecutar cat\n");
            }
            else
            {
                visualizar_contenido_archivo(argumento);
            }
        }
        else if (strcmp(comando, "unlink") == 0)
        {
            if (argumento[0] == '\0')
            {
                fprintf(stderr, "Uso: unlink: <nombre_archivo> \n");
                fprintf(stderr, "Error al ejecutar unlink\n");
            }
            else
            {
                borrar_archivo(argumento);
            }
        }
        else if (strcmp(comando, "find") == 0)
        {
            if (argumento[0] == '\0')
            {
                fprintf(stderr, "Uso: find <ruta> <nombre_archivo>\n");
                fprintf(stderr, "Error al ejecutar find\n");
            }
            else
            {
                encontrar_archivo_normal(argumento, argumento_aux);
            }
        }
        else if (strcmp(comando, "uname") == 0)
        {
            mostrar_info_sistem(bandera);
        }
        else if (strcmp(comando, "date") == 0)
        {
            obtener_date();
        }
        else if (strcmp(comando, "who") == 0)
        {
            obtener_conectividad();
        }
        else if (strcmp(comando, "ip") == 0)
        {
            obtener_ips_tarjetas();
        }
        else if (strcmp(comando, "mac") == 0)
        {
            obtener_mac_tarjetas();
        }
        else if (strcmp(comando, "wall") == 0 && flag_sudo == 1)
        {
            enviar_mensaje_wall(argumento);
        }
        else if (strcmp(comando, "free") == 0)
        {
            mostrar_free();
        }
        else if (strcmp(comando, "numerosdisp") == 0)
        {
            if (argumento[0] == '\0')
            {
                fprintf(stderr, "Uso: numerosdisp <ruta_dispositivo>\n");
            }
            else
            {
                mostrar_numeros_dispositivo(argumento);
            }
        }
        else if (strcmp(comando, "mesgc") == 0)
        {
            if (argumento[0] == '\0' || argumento_aux[0] == '\0')
            {
                fprintf(stderr, "usar: mesg <usuario> <mensaje>\n");
            }
            else
            {
                enviar_por_cola(argumento, argumento_aux);
            }
        }
        else if (strcmp(comando, "leer") == 0)
        {
            if (argumento[0] == '\0')
            {
                fprintf(stderr, "usar: leer <usuario>");
            }
            else
            {
                leer_buzon(argumento);
            }
        }
        else if (strcmp(comando, "findr") == 0)
        {
            if (argumento[0] == '\0' || argumento_aux[0] == '\0')
            {
                fprintf(stderr, "Uso: findr <ruta> <nombre_archivo>\n");
            }
            else
            {
                encontrar_archivo_recursivo(argumento, argumento_aux);
            }
        }
        else if (strcmp(comando, "mesg") == 0)
        {
            if (argumento[0] == '\0' || argumento_aux[0] == '\0')
            {
                fprintf(stderr, "Uso: mesg <usuario> <mensaje>\n");
            }
            else
            {
                enviar_mesg(argumento, argumento_aux);
            }
        }
        else
        {
            fprintf(stderr, "Comando no reconocido: %s\n", comando);
        }
    }
    return 0;
}

// leer
void leer_buzon(char *mi_nombre)
{
    key_t llave;
    int qid;
    struct msgbuf msg;

    // Generamos la misma llave que usó el emisor
    llave = ftok("/tmp", mi_nombre[0]);

    if ((qid = msgget(llave, 0666)) == -1)
    {
        printf("No hay mensajes pendientes para el buzón de: %s\n", mi_nombre);
        return;
    }

    // IPC_NOWAIT evita que la shell se congele si no hay mensajes
    if (msgrcv(qid, (void *)&msg, sizeof(msg.mtext), 1, IPC_NOWAIT) == -1)
    {
        if (errno == ENOMSG)
        {
            printf("El buzón está vacío.\n");
        }
        else
        {
            perror("Error al leer el buzón");
        }
    }
    else
    {
        printf("Contenido del buzón de mensaje: %s\n", msg.mtext);
        printf("--------------------------------\n");
    }
}

// mesg tty
void enviar_mesg(char *usuario, char *mensaje)
{
    if (usuario[0] == '\0' || mensaje[0] == '\0')
    {
        fprintf(stderr, "Uso: mesg <usuario> <mensaje>\n");
        return;
    }

    char comando_sistema[PATH_MAX];

    snprintf(comando_sistema, sizeof(comando_sistema),
             "echo -e \"\r\n\007*** MENSAJE PRIVADO DE LA SHELL ***\r\n%s\r\n\" | sudo write %s",
             mensaje, usuario);

    if (system(comando_sistema) == -1)
    {
        perror("Error al ejecutar mesg");
    }
    else
    {
        printf("Intento de envío realizado a %s\n", usuario);
    }
}

// mesg
void enviar_por_cola(char *nombre_usuario, char *texto)
{
    int qid;
    key_t llave;
    struct msgbuf msg;

    llave = ftok("/tmp", nombre_usuario[0]);

    if ((qid = msgget(llave, IPC_CREAT | 0666)) == -1)
    {
        perror("msgget (enviar)");
        return;
    }

    msg.mtype = 1; // Tipo de mensaje estándar
    snprintf(msg.mtext, sizeof(msg.mtext), "%s", texto);

    if (msgsnd(qid, (void *)&msg, sizeof(msg.mtext), IPC_NOWAIT) == -1)
    {
        perror("msgsnd");
    }
    else
    {
        printf("Mensaje puesto en la cola de %s\n", nombre_usuario);
    }
}

// mostrat numeros
void mostrar_numeros_dispositivo(char *path)
{
    struct stat sb;

    // stat obtiene información sobre el archivo de dispositivo
    if (stat(path, &sb) == -1)
    {
        perror("error al leer dispositivo");
        return;
    }

    // Verificar si es un dispositivo de bloques o caracteres
    printf("Archivo: %s\n", path);
    if (S_ISCHR(sb.st_mode))
    {
        printf("Tipo: Dispositivo de caracteres (Char)\n");
    }
    else if (S_ISBLK(sb.st_mode))
    {
        printf("Tipo: Dispositivo de bloques (Block)\n");
    }
    else
    {
        printf("Tipo: No es un dispositivo de bloques o caracteres\n");
        // exit(EXIT_FAILURE);
        return;
    }

    // major() y minor() extraen los números correspondientes
    printf("Número Mayor (Major): %u\n", major(sb.st_rdev));
    printf("Número Menor (Minor): %u\n", minor(sb.st_rdev));
}

// free
void mostrar_free()
{
    struct sysinfo info;

    if (sysinfo(&info) != 0)
    {
        perror("Error al obtener información del sistema");
        return;
    }

    unsigned long total = (info.totalram * info.mem_unit) / 1024;
    unsigned long libre = (info.freeram * info.mem_unit) / 1024;
    unsigned long compartida = (info.sharedram * info.mem_unit) / 1024;
    unsigned long buffer = (info.bufferram * info.mem_unit) / 1024;
    unsigned long total_swap = (info.totalswap * info.mem_unit) / 1024;
    unsigned long libre_swap = (info.freeswap * info.mem_unit) / 1024;

    // Memoria usada aproximadamente
    unsigned long usada = total - libre;
    unsigned long disponible = libre + buffer;

    printf("\n               total        usada        libre      compart.     buff/cache    disponible\n");
    printf("Mem:    %12lu %12lu %12lu %12lu %12lu %12lu\n",
           total, usada, libre, compartida, buffer, disponible);
    printf("Inter:   %12lu %12lu %12lu\n",
           total_swap, (total_swap - libre_swap), libre_swap);
}

// wall
void enviar_mensaje_wall(char *mensaje)
{
    if (mensaje == NULL || strlen(mensaje) == 0)
    {
        fprintf(stderr, "wall: error, el mensaje está vacío\n");
        return;
    }

    char comando_sistema[RUTA];

    // snprintf(comando_sistema, sizeof(comando_sistema), "wall \"%s\"", mensaje);
    snprintf(comando_sistema, sizeof(comando_sistema), "sudo wall \"%s\"", mensaje);
    printf("Difundiendo mensaje a todos los usuarios...\n");

    if (system(comando_sistema) == -1)
    {
        perror("Error al ejecutar el comando wall");
    }
}

void obtener_mac_tarjetas()
{
    int sock, num_interfaces, i;
    struct ifconf ifc;
    struct ifreq ifr[16]; // Espacio para hasta 16 tarjetas de red
    struct ifreq ifr_mac;
    unsigned char *mac;
    char *nombre_tarjeta;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("Error al crear el socket");
        return;
    }
    ifc.ifc_len = sizeof(ifr);
    ifc.ifc_req = ifr;

    /// pedimos cuantas y cuales interfaces hay
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
    {
        perror("Error al obtener la lista de interfaces");
        close(sock);
        return;
    }
    num_interfaces = ifc.ifc_len / sizeof(struct ifreq);

    for (i = 0; i < num_interfaces; i++)
    {
        nombre_tarjeta = ifr[i].ifr_name;

        // preparamos una estructura limpia solo para preguntar por la MAC de esta tarjeta
        // struct ifreq ifr_mac;
        // Copiamos el nombre de la tarjeta (ej. eth0) a esta nueva estructura
        strncpy(ifr_mac.ifr_name, nombre_tarjeta, IFNAMSIZ - 1);
        ifr_mac.ifr_name[IFNAMSIZ - 1] = '\0';

        // le pedimos al kernel la MAC (SIOCGIFHWADDR) de esta tarjeta específica
        if (ioctl(sock, SIOCGIFHWADDR, &ifr_mac) == -1)
        {
            perror("Error al obtener la dirección MAC");
            continue; // Si falla esta, que siga intentando con la siguiente tarjeta
        }

        // realizAMOS un casteo a unsigned char para que no imprima números negativos.
        mac = (unsigned char *)ifr_mac.ifr_hwaddr.sa_data;

        // 6. Imprimimos usando el formato clásico hexadecimal (XX:XX:XX:XX:XX:XX)
        printf("%s: \t%02X:%02X:%02X:%02X:%02X:%02X\n",
               nombre_tarjeta,
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    close(sock);
}

// ip para obtener las ips de las tarjetas de red
void obtener_ips_tarjetas()
{
    // creamos un canal para comunicarnos con el sistema de red
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("Error al crear el socket");
        return;
    }

    struct ifconf ifc;
    struct ifreq ifr[16]; // Espacio para hasta 16 tarjetas de red

    // decimos dónde y cuánto espacio tiene
    ifc.ifc_len = sizeof(ifr);
    ifc.ifc_req = ifr;

    // pedimos la lista de interfaces al kernel
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
    {
        perror("Error al obtener la configuración de interfaces");
        close(sock);
        return;
    }

    // ifc.ifc_len se actualizó con el tamaño total en bytes de lo que nos devolvió.
    // Lo dividimos entre el tamaño de una sola estructura para saber cuántas son.
    int num_interfaces = ifc.ifc_len / sizeof(struct ifreq);

    // recorremos cada tarjeta encontrada
    for (int i = 0; i < num_interfaces; i++)
    {
        // obtenemos el nombre de la tarjeta (ej. eth0, wlan0, lo)
        char *nombre_tarjeta = ifr[i].ifr_name;

        // obtenemos la dirección IP haciendo el "casteo" a sockaddr_in
        struct sockaddr_in *direccion_ip = (struct sockaddr_in *)&ifr[i].ifr_addr;

        // convertimos los bytes locos de red a un texto legible (ej. 192.168.1.10)
        char *ip_texto = inet_ntoa(direccion_ip->sin_addr);

        // imprimimos el resultado bien formateado
        printf("%s: \t%s\n", nombre_tarjeta, ip_texto);
    }

    // cerrar la puerta al salir
    close(sock);
}

// who
void obtener_conectividad()
{
    struct utmp *entry;
    time_t login_time = time(NULL);
    struct tm *tm_info;
    char buffer[100];

    setutent(); // reinicia la lectura del archivo utmp

    // printf("Usuarios actualmente conectados:\n");
    while ((entry = getutent()) != NULL)
    {
        if (entry->ut_type == USER_PROCESS)
        { // Solo mostramos procesos de usuario
            // extraemos el tiempo
            login_time = entry->ut_tv.tv_sec;
            tm_info = localtime(&login_time);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

            // printf("%s\t %s\t %s\n", entry->ut_user, entry->ut_line, entry->ut_host);
            printf("%s\t %s\t %s\t (%s)\n", entry->ut_user, entry->ut_line, buffer, entry->ut_host);
        }
    }

    endutent(); // Cierra el archivo utmp
}
// date
void obtener_date()
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char buffer[100];
    setlocale(LC_TIME, "es_MX.UTF-8"); // Establece la configuración regional para formatear la fecha correctamente
    strftime(buffer, sizeof(buffer), "%a %d %b %Y %H:%M:%S %Z", tm_info);
    printf("%s\n", buffer);
}
// uname
void mostrar_info_sistem(char *bandera)
{
    struct utsname info;

    // bandera -a
    int flag_a = 0;
    // activamosda bandera -a
    if (bandera != NULL && bandera[0] == '-' && strchr(bandera, 'a') != NULL)
        flag_a = 1;

    if (uname(&info) == -1)
    {
        perror("Error al obtener información del sistema");
        return;
    }
    else
    {
        if (flag_a == 1)
        {
            printf("Sistema Operativo: %s\n", info.sysname);
            printf("Nombre del nodo: %s\n", info.nodename);
            printf("Versión del kernel: %s\n", info.release);
            printf("Arquitectura: %s\n", info.machine);
        }
        else
        {
            printf("%s\n", info.sysname);
        }
    }
}

// find normal
void encontrar_archivo_normal(char *path, char *name)
{
    DIR *dir;
    struct dirent *entrada;
    struct stat info;
    char full_path[1024];

    if (!(dir = opendir(path)))
    {
        perror("Error al abrir directorio");
        return;
    }

    printf("Buscando en: %s (Modo no recursivo)\n", path);

    while ((entrada = readdir(dir)) != NULL)
    {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entrada->d_name);

        if (lstat(full_path, &info) == -1)
            continue;

        if (S_ISDIR(info.st_mode))
        {
            printf("Directorio encontrado PENDIENTE por entrar: %s/ \n", entrada->d_name);
        }
        else if (strcmp(entrada->d_name, name) == 0)
        {
            printf("Archivo encontrado: %s\n", full_path);
        }
    }

    closedir(dir);
}
void encontrar_archivo_recursivo(char *path, char *name)
{
    DIR *dir;
    struct dirent *entrada;
    struct stat info;
    char full_path[PATH_MAX];

    if (!(dir = opendir(path)))
    {
        return;
    }

    while ((entrada = readdir(dir)) != NULL)
    {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
        {
            continue;
        }
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entrada->d_name);

        // Obtener información
        if (lstat(full_path, &info) == -1)
        {
            continue;
        }

        if (strcmp(entrada->d_name, name) == 0)
        {
            printf(" Encontrado: %s\n", full_path);
        }

        if (S_ISDIR(info.st_mode))
        {
            encontrar_archivo_recursivo(full_path, name);
        }
    }

    closedir(dir);
}
int encontrar_archivo(char *path, char *name)
{
    char ruta_actual[RUTA];
    char full_path[PATH_MAX];
    DIR *dir;
    struct dirent *entrada;
    // int bandera = 0; //bandera para indicar si se encontro el archivo

    if (path == NULL)
    {
        if (getcwd(ruta_actual, sizeof(ruta_actual)) == NULL)
        {
            perror("No se puede obtener la ruta actual");
            return 0;
        }
        path = ruta_actual;
    }

    dir = opendir(path);
    if (dir == NULL)
    {
        perror("Error al abrir el directorio");
        return 0;
    }
    // mienr
    while ((entrada = readdir(dir)) != NULL)
    {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
        {
            continue;
        }

        /*strncpy(full_path, path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';

        strcat(full_path, "/");
        strcat(full_path, entrada->d_name);*/

        // conmstruye "path/entrada->d_name"
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entrada->d_name);

        struct stat info;
        if (lstat(full_path, &info) == -1)
        {
            perror("Error al obtener información");
            continue;
        }

        if (S_ISDIR(info.st_mode))
        {
            printf("Pendiente para entrar: %s\n", full_path);
            if (strcmp(entrada->d_name, name) == 0)
            {
                printf("Encontrado: %s\n", full_path);
            }
            encontrar_archivo(full_path, name);
        }
        else
        {
            if (strcmp(entrada->d_name, name) == 0)
            {
                printf("Archivo encontrado: %s\n", full_path);
                // bandera = 1; //se encontro el archivo
                closedir(dir);
                return 1; // salimos de la función si se encontro el archivo
            }
        }
    }

    closedir(dir);
    return 0;
}
// unlink
void borrar_archivo(char *path)
{
    if (path == NULL)
    {
        fprintf(stderr, "unlink: falta el nombre del archivo\n");
        return;
    }
    if (unlink(path) == 0)
    { // devuekve 0 si se borro el archivo con exito y -1 si hubo un error
        printf("Archivo '%s' borrado con éxito.\n", path);
    }
    else
    {
        perror("Error al borrar el archivo");
    }
}

// rename
void renombrar_archivo(char *old_path, char *new_path)
{
    if (new_path == NULL)
    {
        fprintf(stderr, "rename: falta el nuevo nombre del archivo\n");
        return;
    }

    if (rename(old_path, new_path) == 0)
    {
        printf("Archivo '%s' renombrado a '%s' con éxito.\n", old_path, new_path);
    }
    else
    {
        perror("Error al renombrar el archivo");
    }
}

// vfstat
void mostrar_info_sistema_archivos(char *path)
{
    // Esta es la estructura que guardará los datos del disco/partición
    struct statvfs info_fs;

    // Llamamos a statvfs (fíjate que le pasamos el path del archivo,
    // pero la función solita deduce en qué partición está ese archivo)
    if (statvfs(path, &info_fs) == -1)
    {
        perror("Error al obtener información del sistema de archivos");
        return;
    }

    printf("--- Info del Sistema de Archivos donde reside '%s' ---\n", path);

    // Los datos crudos (Directamente de la estructura)
    printf("Tamaño del bloque: %lu bytes\n", info_fs.f_bsize);
    printf("Tamaño del fragmento: %lu bytes\n", info_fs.f_frsize);
    printf("Longitud máxima para un nombre de archivo: %lu caracteres\n", info_fs.f_namemax);

    printf("\nEstadísticas de Bloques:\n");
    printf("Total de bloques en la partición: %lu\n", info_fs.f_blocks);
    printf("Bloques libres: %lu\n", info_fs.f_bfree);
    printf("Bloques disponibles (para usuarios no root): %lu\n", info_fs.f_bavail);

    printf("\nEstadísticas de Inodos (Archivos):\n");
    printf("Total de inodos (archivos máximos permitidos): %lu\n", info_fs.f_files);
    printf("Inodos libres (cuántos archivos más se pueden crear): %lu\n", info_fs.f_ffree);

    // --- EL TOQUE PROFESIONAL (Matemáticas chulas) ---
    // Calculamos el espacio en bytes multiplicando bloques por el tamaño del bloque (f_frsize)
    // Usamos unsigned long long para evitar que el número se desborde si el disco es de muchos Terabytes
    unsigned long long espacio_total = (unsigned long long)info_fs.f_blocks * info_fs.f_frsize;
    unsigned long long espacio_libre = (unsigned long long)info_fs.f_bfree * info_fs.f_frsize;

    // Lo convertimos a Megabytes (MB) dividiendo entre 1024 dos veces (bytes -> KB -> MB)
    printf("\nResumen Amigable (En Megabytes):\n");
    printf("Capacidad Total: %llu MB\n", espacio_total / (1024 * 1024));
    printf("Espacio Libre: %llu MB\n", espacio_libre / (1024 * 1024));
}
// cat
void visualizar_contenido_archivo(char *path)
{
    FILE *file = fopen(path, "r");
    char buffer[PATH_MAX];
    int tipo_archivo = verificar_archivo_reg_ordi(path);
    if (file == NULL)
    {
        perror("Error al abrir el archivo");
        return;
    }
    // verificamso que el archivo sea un archivo regular
    if (0 > tipo_archivo)
    { // si regresa 0 es un directorio y si refgresa 1 es un archivo regular
        perror("cat: no es un archivo regular \n");
        fclose(file);
        return;
    }

    // lee el archivo linea por linea con read

    // printf("%s", buffer);
    // imprimimos con write para evitar problemas con caracteres especiales
    // write(STDOUT_FILENO, buffer, strlen(buffer));

    // imprimimos tipo de archivo
    if (tipo_archivo == 1)
    {
        // write(STDOUT_FILENO, "cat: %s:  archivo regular",*path);
        printf("cat: %s:  archivo regular\n", path);
        // abrimos el archivo con read y lo leemos linea por linea con write
        int fd = fileno(file);
        ssize_t n;
        while ((n = read(fd, buffer, sizeof(buffer))) > 0)
        {
            write(STDOUT_FILENO, buffer, n);
        }
        if (n < 0)
        {
            perror("Error reading file");
        }
    }
    else if (tipo_archivo == 0)
    {
        // write(STDOUT_FILENO, "cat: %s:  directorio",*path);
        printf("cat: %s:  directorio\n", path);
    }

    fclose(file);
}
// verificamos el tipo de archivo para cat
int verificar_archivo_reg_ordi(char *path)
{
    struct stat info;

    if (stat(path, &info) == -1)
    {
        perror("Error al obtener la información del archivo");
        return -1;
    }

    if (S_ISREG(info.st_mode))
    {
        return 1; // Es un archivo regular
    }
    else if (S_ISDIR(info.st_mode))
    {
        return 0; // Es un directorio
    }
    else
    {
        return -1; // No es ni un archivo regular ni un directorio
    }
}
// pwd imprime la ruta actual
void pwd(char *r)
{
    if (getcwd(r, RUTA) == NULL)
    {
        perror("No se puede leer la ruta actual");
        exit(EXIT_FAILURE);
    }
    printf("%s\n", r);
}

// ls lista el contenido
void listar_directorio(char *path, char *bandera)
{
    char ruta_actual[RUTA];
    const char *dir_path = path;
    struct dirent *entry;

    // banderas
    int flag_a = 0; // Para mostrar ocultos (.)
    int flag_l = 0; // Para formato largo
    int flag_i = 0; // Para número de inodo

    // verificamos que banderas estan activas
    if (bandera[0] == '-')
    {
        if (strchr(bandera, 'a') != NULL)
            flag_a = 1;
        if (strchr(bandera, 'l') != NULL)
            flag_l = 1;
        if (strchr(bandera, 'i') != NULL)
            flag_i = 1;
    }

    if (path == NULL)
    {
        if (getcwd(ruta_actual, sizeof(ruta_actual)) == NULL)
        {
            perror("No se puede obtener la ruta actual");
            return;
        }
        dir_path = ruta_actual;
    }

    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("Error al abrir el directorio");
        return;
    }

    printf("Contenido del directorio '%s':\n", dir_path);

    // 2. Leemos las entradas
    while ((entry = readdir(dir)) != NULL)
    {
        // logica bandera -a
        if (flag_a == 0 && entry->d_name[0] == '.')
        {
            continue;
        }
        // armamos una ruta completa que pueda usar stat
        char ruta_completa[PATH_MAX];
        // validamos que la ruta no se desborde
        if (snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", dir_path, entry->d_name) >= sizeof(ruta_completa))
        {
            fprintf(stderr, "Error: La ruta es demasiado larga para procesar\n");
            continue;
        }
        // usamos snprintf para evitar desbordamientos de buffer
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", dir_path, entry->d_name);

        struct stat info;
        if (stat(ruta_completa, &info) == -1)
        {
            perror("Error al obtener información del archivo");
            continue;
        }

        // deteminamos los colores
        char *color = colores[4].color; // color por defecto
        if (S_ISLNK(info.st_mode))
            color = colores[3].color; // enlace
        else if (S_ISDIR(info.st_mode))
            color = colores[0].color; // directorio
        else if (S_ISREG(info.st_mode) && (info.st_mode & S_IXUSR))
            color = colores[2].color; // ejecutable
        else if (S_ISREG(info.st_mode))
            color = colores[1].color; // archivo regular

        //  logica bandera -i
        if (flag_i == 1)
        {
            // Imprimimos el inodo sin salto de línea
            printf("%lu ", entry->d_ino);
        }

        //  logica bandera -l
        if (flag_l == 1)
        {
            // imprimimos los permisos
            char permisos[11];

            // tipo de archivo
            if (S_ISDIR(info.st_mode))
                permisos[0] = 'd';
            else if (S_ISLNK(info.st_mode))
                permisos[0] = 'l';
            else
                permisos[0] = '-';

            // permisosn de usuaerio
            if (info.st_mode & S_IRUSR)
                permisos[1] = 'r';
            else
                permisos[1] = '-';
            if (info.st_mode & S_IWUSR)
                permisos[2] = 'w';
            else
                permisos[2] = '-';
            if (info.st_mode & S_IXUSR)
                permisos[3] = 'x';
            else
                permisos[3] = '-';

            // permisos de grupo
            if (info.st_mode & S_IRGRP)
                permisos[4] = 'r';
            else
                permisos[4] = '-';
            if (info.st_mode & S_IWGRP)
                permisos[5] = 'w';
            else
                permisos[5] = '-';
            if (info.st_mode & S_IXGRP)
                permisos[6] = 'x';
            else
                permisos[6] = '-';

            // permiosos de otros
            if (info.st_mode & S_IROTH)
                permisos[7] = 'r';
            else
                permisos[7] = '-';
            if (info.st_mode & S_IWOTH)
                permisos[8] = 'w';
            else
                permisos[8] = '-';
            if (info.st_mode & S_IXOTH)
                permisos[9] = 'x';
            else
                permisos[9] = '-';
            permisos[10] = '\0'; // terminamos la cadena de permisos

            // tiempo
            char tiempo[20];
            struct tm *tm_info = localtime(&info.st_mtime);
            strftime(tiempo, sizeof(tiempo), "%b %d %H:%M", tm_info);
            // traducimos los id a nombres
            struct passwd *pw = getpwuid(info.st_uid);
            // si pw no es null envcontro el usuario
            char *nombre_usuario = (pw != NULL) ? pw->pw_name : "desconocido";
            // buscar el nombre del grupo
            struct group *gr = getgrgid(info.st_gid);
            char *nombre_grupo = (gr != NULL) ? gr->gr_name : "desconocido";

            // imprimimos la info
            printf("%s %ld %s %s %8ld %s ",
                   permisos, info.st_nlink, nombre_usuario, nombre_grupo, info.st_size, tiempo);
        }

        // Imprimimos el nombre del archivo con colores
        printf("%s%s\033[0m\n", color, entry->d_name);
    }

    closedir(dir);
}

// crear directorio con mkdir
void crear_directorio(char *path)
{
    if (path == NULL)
    {
        fprintf(stderr, "Uso: mkdir <nombre_nuevo_dir>\n");
    }
    else
    {
        if (mkdir(path, DIR_PERMS) == 0)
        {
            printf("Directorio '%s' creado con éxito.\n", path);
        }
        else
        {
            perror("Error al crear el directorio");
        }
    }
}

// cd
void cambiar_directorio(char *path)
{
    if (path == NULL)
    {
        if (chdir(getenv("HOME")) == 0)
        {
            printf("Directorio cambiado a HOME con éxito.\n");
        }
        else
        {
            perror("Error al cambiar al directorio HOME");
        }
    }
    else
    {
        if (chdir(path) == 0)
        {
            printf("Directorio cambiado con éxito.\n");
        }
        else
        {
            perror("Error al cambiar de directorio");
        }
    }
}

// historial de comandos
void historial_comandos()
{
    int inicio, cantidad;

    if (total_comandos < MAX_HIST)
    {
        // El buffer no se ha llenado completamente
        inicio = 0;
        cantidad = total_comandos;
    }
    else
    {
        // El buffer se ha llenado, damos la vuelta
        inicio = posicion_actual;
        cantidad = MAX_HIST;
    }

    // printf("Historial de comandos:\n");
    for (int i = 0; i < cantidad; i++)
    {
        int index = (inicio + i) % MAX_HIST; // Calcula el índice circular
        printf("%d. %s\n", i + 1, historial[index]);
    }
}
// stat
void mostrar_info_archivo(char *path)
{
    struct stat info;

    if (stat(path, &info) == -1)
    {
        perror("Error al obtener la información del archivo");
        return;
    }
    printf("Información del archivo '%s':\n", path);
    printf("Tamaño: %ld bytes\n", info.st_size);
    printf("Permisos: %o\n", info.st_mode & 0777);
    printf("Número de enlaces: %ld\n", info.st_nlink);
    printf("ID del propietario: %d\n", info.st_uid);
    printf("ID del grupo: %d\n", info.st_gid);
    printf("Último acceso: %s", ctime(&info.st_atime));
    printf("Última modificación: %s", ctime(&info.st_mtime));
}
