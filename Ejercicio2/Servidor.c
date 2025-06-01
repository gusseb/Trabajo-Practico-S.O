// servidor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define PUERTO 5000
#define MAX_CLIENTES 4

int clientes_activos = 0;
int servidor_activo = 1;
int servidor_fd;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void procesar_mensaje(char* buffer, char* respuesta) {
    char arg1[64], arg2[64];

    if (strncmp(buffer, "CORTAR:", 7) == 0) {
        sscanf(buffer + 7, "%s", arg1);
        sprintf(respuesta, "Resultado: %s cortada\n", arg1);
    } else if (strncmp(buffer, "COCINAR:", 8) == 0) {
        sscanf(buffer + 8, "%s", arg1);
        sprintf(respuesta, "Resultado: %s cocinada\n", arg1);
    } else if (strncmp(buffer, "EMPLATAR:", 9) == 0) {
        sscanf(buffer + 9, "%s", arg1);
        sprintf(respuesta, "Resultado: %s emplatada\n", arg1);
    } else if (strncmp(buffer, "SUMA:", 5) == 0) {
        sscanf(buffer + 5, "%[^:]:%s", arg1, arg2);
        int a = atoi(arg1), b = atoi(arg2);
        sprintf(respuesta, "Resultado: %d\n", a + b);
    } else if (strncmp(buffer, "RESTA:", 6) == 0) {
        sscanf(buffer + 6, "%[^:]:%s", arg1, arg2);
        int a = atoi(arg1), b = atoi(arg2);
        sprintf(respuesta, "Resultado: %d\n", a - b);
    } else if (strncmp(buffer, "INVERSO:", 8) == 0) {
        sscanf(buffer + 8, "%s", arg1);
        int len = strlen(arg1);
        for (int i = 0; i < len; i++) {
            respuesta[i] = arg1[len - 1 - i];
        }
        respuesta[len] = '\0';
        strcat(respuesta, "\n");
    } else if (strncmp(buffer, "MAYUS:", 6) == 0) {
        sscanf(buffer + 6, "%s", arg1);
        for (int i = 0; arg1[i]; i++) {
            respuesta[i] = toupper(arg1[i]);
        }
        respuesta[strlen(arg1)] = '\0';
        strcat(respuesta, "\n");
    } else {
        strcpy(respuesta, "Comando desconocido.\n");
    }
}

void* manejar_cliente(void* arg) {
    int cliente_fd = *(int*)arg;
    free(arg);

    char buffer[1024], respuesta[1024];
    int leido;

    while ((leido = read(cliente_fd, buffer, sizeof(buffer)-1)) > 0) {
        buffer[leido] = '\0';

        if (strncmp(buffer, "SALIR", 5) == 0) break;

        printf("[Servidor] Recibido: %s", buffer);
        memset(respuesta, 0, sizeof(respuesta));
        procesar_mensaje(buffer, respuesta);
        write(cliente_fd, respuesta, strlen(respuesta));
    }

    close(cliente_fd);
    printf("[Servidor] Cliente desconectado.\n");

    pthread_mutex_lock(&lock);
    clientes_activos--;
    if (clientes_activos == 0) {
        servidor_activo = 0;
        shutdown(servidor_fd, SHUT_RDWR);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main() {
    struct sockaddr_in servidor_addr, cliente_addr;
    socklen_t cliente_len = sizeof(cliente_addr);

    servidor_fd = socket(AF_INET, SOCK_STREAM, 0);
    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servidor_addr.sin_port = htons(PUERTO);

    bind(servidor_fd, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr));
    listen(servidor_fd, MAX_CLIENTES);

    printf("[Servidor] Escuchando en puerto %d...\n", PUERTO);

    while (servidor_activo) {
        int nuevo_fd = accept(servidor_fd, (struct sockaddr*)&cliente_addr, &cliente_len);
        if (nuevo_fd == -1) {
            if (!servidor_activo) break;
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&lock);
        if (clientes_activos >= MAX_CLIENTES) {
            pthread_mutex_unlock(&lock);
            printf("[Servidor] Límite alcanzado. Cliente rechazado.\n");
            close(nuevo_fd);
            continue;
        }
        clientes_activos++;
        pthread_mutex_unlock(&lock);

        int* nuevo_socket = malloc(sizeof(int));
        *nuevo_socket = nuevo_fd;

        pthread_t hilo;
        pthread_create(&hilo, NULL, manejar_cliente, nuevo_socket);
        pthread_detach(hilo);
    }

    close(servidor_fd);
    printf("[Servidor] Finalizado correctamente.\n");
    return 0;
}
