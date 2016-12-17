#include <iostream>
#include "easy.h"


int create_and_bind(uint16_t port_number)
{
    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (0 > sd) {
        perror("Socket");
        exit(1);
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_number);
    const socklen_t addr_size = sizeof(addr);
    int bs = bind(sd, (const struct sockaddr*) &addr, addr_size);
    if (0 > bs) {
        perror("Bind");
        close(sd);
        exit(1);
    }
    return sd;
}

void make_non_blocking(int sd)
{
    int flags = fcntl(sd, F_GETFL);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);
}
