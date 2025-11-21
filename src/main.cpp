#ifndef UNICODE
#define UNICODE
#endif

#include <iostream>
#include <ctime>
#include <format>


#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>


#define TCP_PORT 11500
#define UDP_PORT 11501

#define MAX_NUMBEROF_CONN 100
#define MAX_EVENTS 200
#define MAX_MESSAGE_LENGTH 1024

static int numberOfConnectionsPerExecution = 0;


int parseCommand(std::string &command);
char *getTime();


int main(){
    // Ignoring SIGPIPE signal.
    signal(SIGPIPE, SIG_IGN);


    // Creating epoll object.
    int efd = epoll_create1(0);
    if(efd == -1){
        std::cerr << "Unable to create epoll object." << std::endl;
        return EXIT_FAILURE;
    }


    // Creating sockets.
    // TCP socket.
    int tsfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    sockaddr_in tsaddr;
    tsaddr.sin_family = AF_INET;
    tsaddr.sin_port   = htons(TCP_PORT);
    tsaddr.sin_addr.s_addr = htons(INADDR_ANY);
    if(bind(tsfd, (struct sockaddr*)&tsaddr, sizeof(tsaddr)) < 0){
        std::cerr << "Unable to open TCP socket." << std::endl;
        return EXIT_FAILURE;
    }
    if((listen(tsfd, MAX_NUMBEROF_CONN)) != 0){
        std::cerr << "Unable to listen to TCP socket." << std::endl;
        return EXIT_FAILURE;
    }


    // UDP socket.
    int usfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    sockaddr_in usaddr;
    usaddr.sin_family = AF_INET;
    usaddr.sin_port   = htons(UDP_PORT);
    usaddr.sin_addr.s_addr = htons(INADDR_ANY);
    if(bind(usfd, (struct sockaddr*)&usaddr, sizeof(usaddr)) < 0){
        std::cerr << "Unable to open UDP socket." << std::endl;
        return EXIT_FAILURE;
    }
    //if((listen(usfd, MAX_NUMBEROF_CONN)) != 0){
    //    std::cerr << "Unable to listen to UDP socket." << std::endl;
    //    return EXIT_FAILURE;
    //}


    // Bind epoll.
    epoll_event tcp_event;
    tcp_event.events = EPOLLIN | EPOLLET;
    tcp_event.data.fd = tsfd;

    epoll_event udp_event;
    udp_event.events = EPOLLIN | EPOLLET;
    udp_event.data.fd = usfd;


    if((epoll_ctl(efd, EPOLL_CTL_ADD, tsfd, &tcp_event)) == -1){
        std::cerr << "Epoll append operation error." << std::endl;
        return EXIT_FAILURE;
    }

    if((epoll_ctl(efd, EPOLL_CTL_ADD, usfd, &udp_event)) == -1){
        std::cerr << "Epoll append operation error." << std::endl;
        return EXIT_FAILURE;
    }


    // Main loop.
    epoll_event events[MAX_EVENTS];
    int numberOfFds = 0;
    for(;;){
        numberOfFds = epoll_wait(efd, events, MAX_EVENTS, -1);
        if(numberOfFds == -1){
            std::cerr << "Epoll operation error." << std::endl;
            return EXIT_FAILURE;
        }

        for(int counter = 0; counter < numberOfFds; ++counter){
            // Work with TCP.
            if(events[counter].data.fd == tsfd){
                socklen_t conAddrSize = sizeof(tsaddr);
                int connSock = accept(tsfd, (struct sockaddr*)&tsaddr, &conAddrSize);
                if(connSock == -1){
                    std::cerr << "Socket error." << std::endl;
                    return EXIT_FAILURE;
                }
                
                char *buffer = new char[MAX_MESSAGE_LENGTH];
                int bytes_read = 0;
                if((bytes_read = recv(connSock, buffer, MAX_MESSAGE_LENGTH, 0)) == -1){
                    std::cerr << "Error occured while receiving data." << std::endl;
                    delete buffer;
                    close(connSock);
                    return EXIT_FAILURE;
                }

                std::string strBuffer(buffer);
                int status = parseCommand(strBuffer);

                if(!status){
                    int bytes_send = 0;
                    if((bytes_send = send(connSock, buffer, bytes_read, 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        delete buffer;
                        close(connSock);
                        return EXIT_FAILURE;
                    }
                    close(connSock);
                    delete buffer;
                }
                else if(status){
                    int bytes_send = 0;
                    char *time_buffer = getTime();
                    if(time_buffer == nullptr){
                        close(connSock);
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if((bytes_send = send(connSock, time_buffer, sizeof(time_buffer), 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        close(connSock);
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    close(connSock);
                    delete buffer;
                }
                else if(status == 2){
                    int bytes_send = 0;
                    std::string stats_buffer = std::format("Number of connections per execution = {}\nNumber of current connections = {}",\
                        numberOfConnectionsPerExecution, numberOfFds);
                    if((bytes_send = send(connSock, stats_buffer.data(), stats_buffer.size(), 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        close(connSock);
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    close(connSock);
                    delete buffer;
                }
                else if(status == 3){
                    int bytes_send = 0;
                    char exit_message[] = "shutdown initiated.";
                    if((bytes_send = send(connSock, exit_message, sizeof(exit_message), 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        close(connSock);
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if(epoll_ctl(efd, EPOLL_CTL_DEL, tsfd, NULL) == -1){
                        std::cerr << "Unable to detach TCP socket from events." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if(epoll_ctl(efd, EPOLL_CTL_DEL, usfd, NULL) == -1){
                        std::cerr << "Unable to detach UDP socket from events." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }

                    if(close(tsfd) == -1){
                        std::cerr << "Unable to close TCP socket." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if(close(usfd) == -1){
                        std::cerr << "Unable to close UDP socket." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    delete buffer;
                    exit(EXIT_SUCCESS);
                }
            }
            else if(events[counter].data.fd == usfd){
                char *buffer = new char[MAX_MESSAGE_LENGTH];
                int bytes_read = 0;
                if((bytes_read = recv(usfd, buffer, MAX_MESSAGE_LENGTH, 0)) == -1){
                    std::cerr << "Error occured while receiving data." << std::endl;
                    delete buffer;
                    return EXIT_FAILURE;
                }

                std::string strBuffer(buffer);
                int status = parseCommand(strBuffer);

                if(!status){
                    int bytes_send = 0;
                    if((bytes_send = send(usfd, buffer, bytes_read, 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    delete buffer;
                }
                else if(status){
                    int bytes_send = 0;
                    char *time_buffer = getTime();
                    if(time_buffer == nullptr){
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if((bytes_send = send(usfd, time_buffer, sizeof(time_buffer), 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    delete buffer;
                }
                else if(status == 2){
                    int bytes_send = 0;
                    std::string stats_buffer = std::format("Number of connections per execution = {}\nNumber of current connections = {}",\
                        numberOfConnectionsPerExecution, numberOfFds);
                    if((bytes_send = send(usfd, stats_buffer.data(), stats_buffer.size(), 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    delete buffer;
                }
                else if(status == 3){
                    int bytes_send = 0;
                    char exit_message[] = "shutdown initiated.";
                    if((bytes_send = send(usfd, exit_message, sizeof(exit_message), 0)) == -1){
                        std::cerr << "Error occured while sending data." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if(epoll_ctl(efd, EPOLL_CTL_DEL, tsfd, NULL) == -1){
                        std::cerr << "Unable to detach TCP socket from events." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if(epoll_ctl(efd, EPOLL_CTL_DEL, usfd, NULL) == -1){
                        std::cerr << "Unable to detach UDP socket from events." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }

                    if(close(tsfd) == -1){
                        std::cerr << "Unable to close TCP socket." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    if(close(usfd) == -1){
                        std::cerr << "Unable to close UDP socket." << std::endl;
                        delete buffer;
                        return EXIT_FAILURE;
                    }
                    delete buffer;
                    exit(EXIT_SUCCESS);
                }
            }
        }
        ++numberOfConnectionsPerExecution;
    }
    

    return EXIT_SUCCESS;
}


int parseCommand(std::string &command){
    if(!command.contains("/")){
        return 0;
    }
    else{
        if(command.contains("/time")){
            return 1;
        }
        else if(command.contains("/stats")){
            return 2;
        }
        else if(command.contains("/shutdown")){
            return 3;
        }
    }
    return 0;
}


char *getTime(){
    int time_size = std::size("yyyy-mm-dd hh-mm-ss");
    char *time_buffer = new char[time_size];
    if(!std::strftime(time_buffer, time_size, "%Y-%m-%d %T", std::localtime(std::time_t()))){
        std::cerr << "Unable to get current time" << std::endl;
        return (char*)(nullptr);
    }
    return time_buffer;
}