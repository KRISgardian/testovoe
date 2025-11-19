#ifndef UNICODE
#define UNICODE
#endif

#include <iostream>


#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>


#define TCP_PORT 11500
#define UDP_PORT 11501

#define MAX_NUMBEROF_CONN 100
#define MAX_EVENTS 200
#define MAX_MESSAGE_LENGTH 1024


int parseCommand(std::string &command);


int main(){
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
    if((listen(usfd, MAX_NUMBEROF_CONN)) != 0){
        std::cerr << "Unable to listen to UDP socket." << std::endl;
        return EXIT_FAILURE;
    }


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
                
                char buffer[MAX_MESSAGE_LENGTH];
                int bytes_read = 0;
                if((bytes_read = recv(connSock, buffer, MAX_MESSAGE_LENGTH, MSG_DONTWAIT)) == -1){
                    std::cerr << "Error occured while receiving data." << std::endl;
                    return EXIT_FAILURE;
                }



                

            }
        }




    }
    

    return EXIT_SUCCESS;
}


int parseCommand(std::string &command){
    if(!command.contains("/")){
        return 0;
    }
    return 0;
}