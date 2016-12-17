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

#include "easy.h"

using namespace std;

static const uint16_t PortNumber = 3000;
static const size_t MaxEvents = 10;
static const size_t QuantumSize = 10 /* bytes */;
static const string Message = "Hello world\n";

static volatile sig_atomic_t StopRequest = 0;
void stop_handler(int s)
{
    StopRequest = 1;
}

int main(int argc, char* argv[])
{
    int status = 0;

    int sock_fd = create_and_bind(PortNumber);

    make_non_blocking(sock_fd);

    status = listen(sock_fd, SOMAXCONN);
    if (0 > status) {
        perror("Listen");
        close(sock_fd);
        exit(1);
    }



    int ed = epoll_create1(0);


    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = sock_fd;
    event.events = EPOLLIN;

    status = epoll_ctl(ed, EPOLL_CTL_ADD, sock_fd, &event);

    if (0 > status) {
        perror("Epoll control / Kqueue kevent");
        close(sock_fd);
        exit(1);
    }

    signal(SIGINT, stop_handler);
    signal(SIGTERM, stop_handler);


    epoll_event *pending_events = new epoll_event[MaxEvents];


    map<int,size_t> out_data_positions;

    while ( ! StopRequest ) {

        int n = epoll_wait(ed, pending_events, MaxEvents, -1);

        if (-1 == n) {
            break; // Bye!
        }
        else if (0 > n) {
            perror("Epoll/Kqueue wait");
            close(sock_fd);
            exit(1);
        }

        for (int i=0; i<n; ++i) {

            const uint32_t emask = pending_events[i].events;
            const bool e_error = emask & EPOLLERR;
            const bool e_hup = emask & EPOLLHUP;
            const bool e_out = emask & EPOLLOUT;
            const bool e_in = emask & EPOLLIN;
            const int fd = pending_events[i].data.fd;

            if ( e_error || e_hup )
                {
                                            
                    if (out_data_positions.count(fd)) {
                        out_data_positions.erase(fd);
                    }
                    close(fd);
                    continue;
                }
            else if (fd == sock_fd) {
                // Incoming connection event
                while (true) {
                    // There is possible several connections at a time,
                    // so accept them all

                    struct sockaddr_in in_addr;
                    socklen_t in_addr_size = sizeof(in_addr);
                    int incoming_fd = accept(sock_fd, (sockaddr*)&in_addr, &in_addr_size);
                    if (-1 == incoming_fd) {
                        // This might be not an error!
                        if (EAGAIN == errno || EWOULDBLOCK == errno) {
                            break;
                        }
                        else {
                            perror("Accept failed");
                            close(sock_fd);
                            exit(1);
                        }
                    }
                    else {
                        // Register newly created file descriptor for
                        // event processing
                        make_non_blocking(incoming_fd);

                        event.data.fd = incoming_fd;
                        event.events = EPOLLIN | EPOLLOUT;
                        status = epoll_ctl(ed, EPOLL_CTL_ADD, incoming_fd, &event);

                        if (0 > status) {
                            perror("Epoll/Kqueue control for incoming connection");
                            close(sock_fd);
                            exit(1);
                        }
                        out_data_positions.insert(make_pair(incoming_fd, 0));
                    }
                    continue;
                } /* end while(true) */

            }
            else if ( e_out ) {
                // Previous data block was successfully sent,
                // and current connection is ready to eat some
                // more data
                size_t last_pos = out_data_positions[fd];

                const string message_quant = Message.substr(last_pos, QuantumSize);
                size_t new_pos = message_quant.length() < QuantumSize
                                                          ? 0
                                                          : last_pos + QuantumSize;
                write(fd, message_quant.c_str(), message_quant.length());
                out_data_positions[fd] = new_pos;
            }
            else if ( e_in ) {
                // Connection wants to write us some data
                // TODO implement me the same way!
            }
            else {
                cerr << "This branch unreachable!" << endl;
                close(sock_fd);
                exit(1);
            }
        }
    }

    delete[] pending_events;

    close(sock_fd);
    cout << "Bye!" << endl;
    return 0;
}
