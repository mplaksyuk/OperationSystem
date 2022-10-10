#include <iostream>
#include <cstdint>
#include <future>
#include <optional>
#include <cmath>

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
    
#define PORT         1234 

#define REQUEST_SIG  0x12345678
#define RESPONSE_SIG 0x87654321

struct request_datagram
{
    std::uint32_t sig;

    double a;
    double b;
    double c;
};

struct response_datagram
{
    std::uint32_t sig;

    double x1;
    double x2;
};

response_datagram quadratic_equation(double a, double b, double c) {
    double D = b * b - 4 * a * c;

    if (D > 0) 
    {
        double x1 = (-b + sqrt(D)) / (2 * a);
        double x2 = (-b - sqrt(D)) / (2 * a);

        return {RESPONSE_SIG, x1, x2};
    } 
    else if (D == 0) 
    {
        double x = (-b) / (2 * a);
        return {RESPONSE_SIG, x, x};
    }
    else
        return {RESPONSE_SIG, std::nan(""), std::nan("")};
}


void run_server()
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor 
    if ((sockfd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        exit(EXIT_FAILURE); 
    }

    // Filling server information 
    std::memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 

    // Bind the socket with the server address 
    if (bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        std::cerr << "Bind failed" << std::endl; 
        exit(EXIT_FAILURE); 
    }

    request_datagram req;

    auto n = ::recvfrom(sockfd, &req, sizeof(req), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);

    if (n == sizeof(req) && req.sig == REQUEST_SIG)
    {
        std::cout << "Request : a = " << req.a << ", b = " << req.b << ", c = " << req.c << std::endl;

        auto v = quadratic_equation(req.a, req.b, req.c);

        std::cout << "x1 = " << v.x1 << ", x2 = " << v.x2 << std::endl;

        ::sendto(sockfd, &v, sizeof(v), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr)); 
    }
    else
    {
        std::cerr << "Unexpected request" << std::endl;
    }

}

std::optional<response_datagram> run_client(double a, double b, double c)
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr; 
    
    // Creating socket file descriptor 
    if ((sockfd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        std::cerr << "socket creation failed" << std::endl;
        exit(EXIT_FAILURE); 
    }
    
    // Filling server information 
    std::memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 

    // Send request
    request_datagram req{ REQUEST_SIG, a, b, c };
    ::sendto(sockfd, &req, sizeof(req), 0, (const struct sockaddr *) &servaddr,  sizeof(servaddr));

    // Receive response
    response_datagram res;
    auto n = ::recvfrom(sockfd, &res, sizeof(res), MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
    ::close(sockfd);

    if (n == sizeof(res) && res.sig == RESPONSE_SIG) 
    {
        return res;
    }
    else
        return { };
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        std::cout << "Running server..." << std::endl;
        run_server();
    }
    else if (argc == 4)
    {
        auto a = std::atof(argv[1]);
        auto b = std::atof(argv[2]);
        auto c = std::atof(argv[3]);

        std::cout << "Running client: a = " << a << ", b = " << b << ", c = " << c << std::endl;
        auto x = std::async(std::launch::async, &run_client, a, b, c);
        auto r = x.get();
        if (r.has_value())
        {
            auto const &v = r.value();
            std::cout << "x1 = " << v.x1 << ", x2 = " << v.x2 << std::endl;
        }
        else
            std::cerr << "Unexpected response" << std::endl;
    }
    else
        std::cerr << "Numbers a b c expected" << std::endl;

    return 0;
}