#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <errno.h>
#include <string>
#include <map>
#include <iostream>

int create_and_bind(uint16_t port_number);
void make_non_blocking(int sd);
