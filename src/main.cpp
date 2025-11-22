#ifndef UNICODE
#define UNICODE
#endif


#include <iostream>
#include <ctime>
#include <cstring>
#include <format>


#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#define TCP_PORT 11500
#define UDP_PORT 11501


#define MAX_NUMBEROF_CONNS 100  // Max number of connections(max length of the queue).
#define MAX_EVENTS 200          // Max number of epoll events(epoll queue).
#define MAX_MESSAGE_LENGTH 1024 // Max length of receiving message.



static int nocpe = 0; // Number of connectiosn per program execution.
static int timeSize = std::size("yyyy-mm-dd hh:mm:ss") + 1; // Size of the time in required format.



int parseInput(std::string &buffer);
char *getTime();



int main(){
    // Creating epoll object.
    int efd = epoll_create1(0);
    if(efd == -1){
        std::cerr << "Unable to create epoll object." << std::endl;
        return EXIT_FAILURE;
    }


    // Creating sockets.
    // TCP socket.
    int tcpfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    
    sockaddr_in tcpaddr;
    socklen_t tcpaddrlen = sizeof(tcpaddr);
    tcpaddr.sin_family      = AF_INET;
    tcpaddr.sin_port        = htons(TCP_PORT);
    tcpaddr.sin_addr.s_addr = htons(INADDR_ANY);

    if(bind(tcpfd, (struct sockaddr*)&tcpaddr, sizeof(tcpaddr)) < 0){
        std::cerr << "Unable to bind TCP socket." << std::endl;
        close(tcpfd);
        close(efd);
        return EXIT_FAILURE;
    }

    if(listen(tcpfd, MAX_NUMBEROF_CONNS) != 0){
        std::cerr << "Unable to listen to TCP socket." << std::endl;
        close(tcpfd);
        close(efd);
        return EXIT_FAILURE;
    }


    // UDP socket.
    int udpfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    
    sockaddr_in udpaddr, udprecaddr;
    socklen_t udpaddrlen    = sizeof(udpaddr);
    socklen_t udprecaddrlen = sizeof(udprecaddr);
    udpaddr.sin_family      = AF_INET;
    udpaddr.sin_port        = htons(UDP_PORT);
    udpaddr.sin_addr.s_addr = htons(INADDR_ANY);

    if(bind(udpfd, (struct sockaddr*)&udpaddr, sizeof(udpaddr)) < 0){
        std::cerr << "Unable to bind UDP socket." << std::endl;
        close(tcpfd);
        close(udpfd);
        close(efd);
        return EXIT_FAILURE;
    }



    // Bind epoll.
    epoll_event tcpEvent;
    tcpEvent.events = EPOLLIN | EPOLLET;
    tcpEvent.data.fd = tcpfd;


    epoll_event udpEvent;
    udpEvent.events = EPOLLIN | EPOLLET;
    udpEvent.data.fd = udpfd;



    if(epoll_ctl(efd, EPOLL_CTL_ADD, tcpfd, &tcpEvent) == -1){
        std::cerr << "Epoll bind operation error." << std::endl;
        close(tcpfd);
        close(udpfd);
        close(efd);
        return EXIT_FAILURE;
    }


    if(epoll_ctl(efd, EPOLL_CTL_ADD, udpfd, &udpEvent) == -1){
        std::cerr << "Epoll bind operation error." << std::endl;
        close(tcpfd);
        close(udpfd);
        close(efd);
        return EXIT_FAILURE;
    }



    // Main loop.
    epoll_event events[MAX_EVENTS];
    int numberOfFds = 0;
    for(;;){
        numberOfFds = epoll_wait(efd, events, MAX_EVENTS, -1);
        if(numberOfFds == -1){
            std::cerr << "Main queue error." << std::endl;
            close(tcpfd);
            close(udpfd);
            close(efd);
            return EXIT_FAILURE;
        }


        for(int counter = 0; counter < numberOfFds; ++counter){
            // Work with TCP.
            if(events[counter].data.fd == tcpfd){
                int connSocket = accept(tcpfd, (struct sockaddr*)&tcpaddr, &tcpaddrlen);
                if(connSocket == -1){
                    std::cerr << "Unable to accept TCP connection." << std::endl;
                    close(tcpfd);
                    close(udpfd);
                    close(efd);
                    break;
                }


                int bytesRead = 0;
                char tcpBuffer[MAX_MESSAGE_LENGTH];
                memset(tcpBuffer, 0, MAX_MESSAGE_LENGTH);


                if((bytesRead = recv(connSocket, tcpBuffer, MAX_MESSAGE_LENGTH, 0)) == -1){
                    std::cerr << "Error occurred while receiving data." << std::endl;
                    close(tcpfd);
                    close(udpfd);
                    close(efd);
                    close(connSocket);
                    return EXIT_FAILURE;
                }


                std::string strBuffer(tcpBuffer);
                int status = parseInput(strBuffer);


                std::cout << "message=" << strBuffer << "status=" << status << std::endl;


                int bytesSend = 0;


                if(status == 0){
                    std::cout << "answer=" << tcpBuffer << std::endl;
                    if((bytesSend = send(connSocket, tcpBuffer, bytesRead, 0)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        close(connSocket);
                        return EXIT_FAILURE;
                    }
                }
                else if(status == 1){
                    char *answer = getTime();
                    std::cout << "answer=" << answer << std::endl;
                    if((bytesSend = send(connSocket, answer, timeSize, 0)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        delete(answer);
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        close(connSocket);
                        return EXIT_FAILURE;
                    }
                    delete(answer);
                }
                else if(status == 2){
                    std::string stats = std::format("Number of connections per execution = {}\nNumber of current connections = {}\n", nocpe, numberOfFds);
                    std::cout << "answer=" << stats << std::endl;
                    if((bytesSend = send(connSocket, stats.data(), stats.size(), 0)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        close(connSocket);
                        return EXIT_FAILURE;
                    }
                }
                else if(status == 3){
                    char exitMessage[] = "shutdown initiated\n";
                    std::cout << "answer=" << exitMessage << std::endl;
                    if((bytesSend = send(connSocket, exitMessage, sizeof(exitMessage), 0)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        close(connSocket);
                        return EXIT_FAILURE;
                    }
                    close(tcpfd);
                    close(udpfd);
                    close(efd);
                    close(connSocket);
                    exit(EXIT_SUCCESS);
                }
                close(connSocket);
            }
            else if(events[counter].data.fd == udpfd){
                int bytesRead = 0;
                char udpBuffer[MAX_MESSAGE_LENGTH];
                memset(udpBuffer, 0, MAX_MESSAGE_LENGTH);


                if((bytesRead = recvfrom(udpfd, udpBuffer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr*)&udprecaddr, &udprecaddrlen)) == -1){
                    std::cerr << "Error occurred while receiving data." << std::endl;
                    close(tcpfd);
                    close(udpfd);
                    close(efd);
                    return EXIT_FAILURE;
                }


                std::string strBuffer(udpBuffer);
                int status = parseInput(strBuffer);


                std::cout << "message=" << strBuffer << "status=" << status << std::endl;


                int bytesSend = 0;


                if(status == 0){
                    std::cout << "answer=" << udpBuffer << std::endl;
                    if((bytesSend = sendto(udpfd, udpBuffer, bytesRead, 0, (struct sockaddr*)&udprecaddr, udprecaddrlen)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        return EXIT_FAILURE;
                    }
                }
                else if(status == 1){
                    char *answer = getTime();
                    std::cout << "answer=" << answer << std::endl;
                    if((bytesSend = sendto(udpfd, answer, timeSize, 0, (struct sockaddr*)&udprecaddr, udprecaddrlen)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        delete(answer);
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        return EXIT_FAILURE;
                    }
                    delete(answer);
                }
                else if(status == 2){
                    std::string stats = std::format("Number of connections per execution = {}\nNumber of current connections = {}\n", nocpe, numberOfFds);
                    std::cout << "answer=" << stats << std::endl;
                    if((bytesSend = sendto(udpfd, stats.data(), stats.size(), 0, (struct sockaddr*)&udprecaddr, udprecaddrlen)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        return EXIT_FAILURE;
                    }
                }
                else if(status == 3){
                    char exitMessage[] = "shutdown initiated\n";
                    std::cout << "answer=" << exitMessage << std::endl;
                    if((bytesSend = sendto(udpfd, exitMessage, sizeof(exitMessage), 0, (struct sockaddr*)&udprecaddr, udprecaddrlen)) == -1){
                        std::cerr << "Error occurred while sending data." << std::endl;
                        close(tcpfd);
                        close(udpfd);
                        close(efd);
                        return EXIT_FAILURE;
                    }
                    close(tcpfd);
                    close(udpfd);
                    close(efd);
                    exit(EXIT_SUCCESS);
                }
            }
            ++nocpe;
        }
    }
    return EXIT_SUCCESS;
}



char *getTime(){
    char *timeBuffer = new char[timeSize];
    std::time_t time = std::time({});
    if(!std::strftime(timeBuffer, timeSize, "%Y-%m-%d %T%n", std::gmtime(&time))){
        std::cerr << "Unable to get current time" << std::endl;
        return nullptr;
    }
    return timeBuffer;
}



int parseInput(std::string &buffer){
    if(!buffer.contains("/")){
        return 0;
    }
    else{
        if(buffer.contains("/time")){
            return 1;
        }
        else if(buffer.contains("/stats")){
            return 2;
        }
        else if(buffer.contains("/shutdown")){
            return 3;
        }
    }
    return 0;
}