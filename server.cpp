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
#include <fstream>
#include <sys/stat.h>

#include <sys/types.h>
#include <curl/curl.h>
#include <curl/easy.h>


using namespace std;

static const uint16_t PortNumber = 3000;
static const size_t MaxEvents = 100;
static volatile sig_atomic_t StopRequest = 0;

static int create_and_bind() {
    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (0 > sd) {
        perror("Socket");
        exit(1);
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PortNumber);
    const socklen_t addr_size = sizeof(addr);
    int bs = bind(sd, (const struct sockaddr*) &addr, addr_size);
    if (0 > bs) {
        perror("Bind");
        close(sd);
        exit(1);
    }
    return sd;
}

static void make_non_blocking(int sd) {
    int flags = fcntl(sd, F_GETFL);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);
}

static void stop_handler(int s) {
    StopRequest = 1;
}

static void starting_server(string &contents) {
    int status = 0;
    int sock_fd = create_and_bind();

    make_non_blocking(sock_fd);

    status = listen(sock_fd, SOMAXCONN);
    if (0 > status) {
        perror("Listen");
        close(sock_fd);
        exit(1);
    }
    /* Create queue */

    int ed = epoll_create1(0);
    /* Add event handler for incoming connections */

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
            const int fd = pending_events[i].data.fd;


            if ( e_error || e_hup )
                {
                    if (e_error)
                        cerr << "Something wrong!";
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
                write(fd, contents.c_str(), contents.length());
                close(fd);
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
}

static void download(string &addr, string &name) {
    CURL *curl;

    curl = curl_easy_init();

    FILE *file;
    mkdir("cache", 0755);
    file = fopen(("cache/" + name).c_str(),"w");
    fprintf(file, "%s\n","HTTP/1.1 200 OK\r\nContent-Type: text/html;charset = UTF-8\r\n\r\n<!DOCTYPE html>\r\n");
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, addr.c_str());
        curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA, file);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

    } else {
        cout << "curl error\n";
        exit(1);
    }
    //file.close;
    fclose(file);

}

static void open_cached_page(string &name) {
    ifstream in("cache/" + name);
    string contents((std::istreambuf_iterator<char>(in)),
                    istreambuf_iterator<char>());

    in.close();

    starting_server(contents);

}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");
    string addr, name;
    addr = argv[1];
    name = argv[2];
    download(addr, name);
    open_cached_page(name);


    return 0;
}
