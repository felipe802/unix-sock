#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // Definições vitais da API de sockets
#include <netinet/in.h> // Estruturas para IPs (como sockaddr_in)
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h> // Funções de traducão de IPs e Endianness (htons)
#include "server.h"

// static esconde essa função. Só pode ser usada dentro do server.c
static void log_server_addresses(int port) 
{
    // Será o ponteiro para o início da lista encadeada do SO
    struct ifaddrs *interfaces;
    // INET_ADDRSTRLEN é uma constante do sistema (vale 16)
    // É o tamanho exato de um IP em texto ("192.168.100.100" + '\0' final)
    char ip_str[INET_ADDRSTRLEN];

    printf("[INFO]: Server started.\n");
    printf("[INFO]: You can access the server clicking on the links below:\n");

    // Pedir a lista de placas de rede ao SO
    // O '&' passa o endereço do nosso ponteiro para a função preenchê-lo
    if (getifaddrs(&interfaces) != 0) 
    { // Quando != 0 é porque deu erro. Êxito retorna 0
        // Se o SO falhar em listar o hardware, nós não deixamos o programa cair
        // Assumimos um "fallback" seguro: imprimimos apenas o localhost e encerramos
        // Imprime apenas o que funcionou, endereço local padrão criado antes
        // localhost = 127.0.0.1
        printf("        -> http://127.0.0.1:%d/\n", port);
        printf("\n[INFO]: Press Ctrl+C or Ctrl+Z to gracefully stop the server.\n\n");
        return;
    }

    // Percorrer a lista encadeada (laço for clássico para linked lists)
    // A partir de 'temp_addr', em cada placa de rede, até nó nulo
    // Agora 'interfaces' contém o endereço real da memória onde a primeira placa
    // de rede está salva, pronta para ser lida no laço 'for' seguinte
    for (struct ifaddrs *temp_addr = interfaces; temp_addr != NULL; temp_addr = temp_addr->ifa_next) 
    { // getifaddrs retornou 0
        // 1. A placa precisa ter um endereço atrbuído (!= NULL)
        // 2. O endereço tem que ser IPv4
        // Sem esse filtro, o código quebra tentando ler placas IPv6 ou Bluetooth
        if (temp_addr->ifa_addr != NULL && temp_addr->ifa_addr->sa_family == AF_INET) {
            // Casting para IPv4
            struct sockaddr_in *sock_addr = (struct sockaddr_in *)temp_addr->ifa_addr;
            // Network to pointer
            // 1. Protocolo, bits crus do IP, onde guardar a string legível, limite do tamanho
            inet_ntop(AF_INET, &sock_addr->sin_addr, ip_str, INET_ADDRSTRLEN);

            // IP formatado com a porta do servidor
            printf("        -> http://%s:%d/\n", ip_str, port);
        }
    }

    // Liberar a memória pega pelo getifaddrs
    freeifaddrs(interfaces);
    printf("\n[INFO]: Press Ctrl+C or Ctrl+Z to gracefully stop the server.\n\n");
}

int server_init(int port) 
{
    int server_socket;
    // Estrutura que guarda nosso "número de telefone" (IP + Porta)
    struct sockaddr_in socket_address = {};
    int socket_opt = 1; // Flag booleana (true) para passar ao setsockopt

    // 1. Criar o socket (aparelho telefónico)
    // AF_INET = IPv4
    // SOCK_STREAM = TCP
    // 0 = SO escohe o protocolo padrão para fazer a combinação
    server_socket = socket(AF_INET, SOCK_STREAM, 0); 
    // Kernel devolve um número negativo se não houver mais memória RAM ou
    // se o limite de arquivos abertos (ulimit -n) do usuário estourar 
    if (server_socket < 0){
        perror("[ERR]: Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 2. Configurar o socket (opção anti-bloqueio, liberar imediatamente)
    // SOL_SOCKET = nível da opção do socket, não do TCP
    // SO_REUSEADDR = Permite reutilizar a porta local imediatamente aós o fecho do programa
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt))){
        perror("[ERR]: Error in setsockopt");
        exit(EXIT_FAILURE);
    }
    printf("\n[INFO]: Socket created: %d\n", server_socket);

    // 3. Preparar o endereço e a porta
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY; // Aceita qualquer conexão da placa de rede, estcutar TODAS as placas de rede da máquina
    // s_addr armazena o IP em formato binário de 32 bits
    socket_address.sin_port = htons(port);
    // Se passar a porta 8080 (0x1F90) sem converter, o Kernel da rede vai ler 36895 (0x901F)

    // 4. Ligar o aparelho à tomada da parede
    // Bind exige um ponteiro genérico 'struct sockaddr *'
    // Pedimos ao SO que associe o server_socket ao endereço e porta configurados
    // C não tem herança de classes como C++
    if (bind(server_socket, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0){
        perror("[ERR]: Error in bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("[INFO]: Bind successful!\n");

    // Ligar o receptáculo
    // De "inativo" para "passivo", agora pode receber chamadas
    // A 256ª pessoa que tentar conexão com o servidor recebe um erro de rede
    if (listen(server_socket, 255) < 0){
        perror("[ERR]: Error in listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Imprime os links no terminal
    log_server_addresses(port);

    // O main.c pega esse número e entrega à função poll() para vigiá-lo
    return server_socket;
}
