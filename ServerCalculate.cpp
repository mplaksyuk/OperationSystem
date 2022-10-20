#include <iostream>
#include <cstdint>
#include <future>
#include <map>
#include <string>

#include <sys/time.h>
#include <cstdlib>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#include "./functions/trialfuncs.hpp"
#include "./CancelationDialog.hpp"

#define PORT 2222
#define TIMEOUT 5s

std::string PROCESS_NAME = "";

bool InProcess::IN_PROCESS = false;
bool InProcess::PROMPT = true;
bool InProcess::STOP_PROCESS = false;
bool InProcess::CONTINUE_PROCESS = false;

Response hard_fail_resp{RESPONSE_SIG_F, 0, "hard_fail", std::nan("hard_fail")};

void attemptFail(int attempt_times)
{
    std::cout << "The function tried to be evaluated " << attempt_times << " times. Break." << std::endl;
}

void binaryOperation(double a, double b)
{
    std::cout << "Binary Operation (*): " << a * b << std::endl;
}

Response f_function(double x)
{
    Response r;
    r << os::lab1::compfuncs::trial_f<os::lab1::compfuncs::DOUBLE_MULT>(x);
    return r;
}

Response g_function(double x)
{
    Response r;
    r << os::lab1::compfuncs::trial_g<os::lab1::compfuncs::DOUBLE_MULT>(x);
    return r;
}

void runProcess(std::string functionName, double x, int iterno) 
{
    std::system((PROCESS_NAME + " " + functionName + " " + std::to_string(x) + " " + std::to_string(iterno) + " &").c_str());
}

void recvfrom_process(int sockfd, sockaddr_in cliaddr, socklen_t len, std::map<std::string, Response> *res, int iterno)
{
    using namespace std::chrono_literals;

    Response response;
    auto ans = 0;

    InProcess::PROMPT = true;
    InProcess::STOP_PROCESS = false;
    InProcess::CONTINUE_PROCESS = false;

    int attempt_times = 0;

    auto expire = std::chrono::system_clock::now() + TIMEOUT;
    do
    {
        len = sizeof(cliaddr);
        ans = ::recvfrom(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &cliaddr, &len);
        if (ans == -1)
        {
            if(InProcess::STOP_PROCESS)
                break; //stop proc was pressed

            if (errno == EINTR)
                continue; //^C was pressed

            if (!InProcess::IN_PROCESS && expire < std::chrono::system_clock::now() && InProcess::PROMPT) 
            {
                if (++attempt_times == 5 && !InProcess::CONTINUE_PROCESS)
                {
                    attemptFail(attempt_times);
                    break;
                }

                InProcess busy;

                std::string user_ans;

                std::cout << "Answer timeout" 
                          << std::endl << "Prompt:"
                          << std::endl << "(a) continue" 
                          << std::endl << "(b) continue without prompt" 
                          << std::endl << "(c) stop" << std::endl;

                std::cin >> user_ans;

                if (user_ans == "a")
                {
                    expire = std::chrono::system_clock::now() + TIMEOUT;
                    continue;
                }
                else if (user_ans == "b")
                {
                    InProcess::PROMPT = false;
                    continue;
                }
                else if (user_ans == "c") 
                    InProcess::STOP_PROCESS = true;
            }

            if (expire < std::chrono::system_clock::now())
            {
                if (++attempt_times == 5 && !InProcess::CONTINUE_PROCESS)
                {
                    attemptFail(attempt_times);
                    break;
                }
                expire = std::chrono::system_clock::now() + TIMEOUT;
            }
        }
        else if (ans > 0 && ans == sizeof(response) && iterno == response.iterno)
        {   
            if (response.sig == RESPONSE_SIG_F || response.sig == RESPONSE_SIG_G)
            {   
                // if (response.status != "ok")
                //     std::cout << "Response failure: " << response.status << std::endl;

                if (response.sig == RESPONSE_SIG_F)
                    res->at("f") = response;
                else
                    res->at("g") = response;
            }
            else if (response.sig != RESPONSE_SIG_F || response.sig != RESPONSE_SIG_G)
            {
                std::cerr << "Signature failed" << std::endl;
                continue;
            }
        }
    }
    while (ans == -1 || res->at("f").status == "hard_fail" || res->at("g").status == "hard_fail");
}

void run_manager()
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;

    if ((sockfd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        exit(EXIT_FAILURE); 

    struct timeval tv { 0, 200000 };
    ::setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) 
    { 
        std::cerr << "Bind failed" << std::endl; 
        exit(EXIT_FAILURE); 
    }

    std::thread thread(&cancelationDialog);

    for(int iterno = 0; true; iterno++)
    {
        {
            InProcess busy;

            int x;

            std::cout << "Enter x: ";
            std::cin >> x;

            runProcess("f", x, iterno);
            runProcess("g", x, iterno);
        }

        std::map<std::string, Response> res {{"f", hard_fail_resp}, {"g", hard_fail_resp}};
        recvfrom_process(sockfd, cliaddr, len, &res, iterno);

        if (res.size() == 2) {
            for (const auto& [key, value] : res)
                std::cout << '[' << key << "] status: "<< value.status << " = " << value.value << "; " << std::endl;;

            if(res["f"].status == "ok" && res["g"].status == "ok")
                binaryOperation(res["f"].value, res["g"].value);
            else
                std::cout << "Value failure!" << std::endl;
        }
    }
}

void run_client(std::string name, int x, int iterno)
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr; 

    if ((sockfd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        std::cerr << "socket creation failed" << std::endl;
        exit(EXIT_FAILURE); 
    }

    std::memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY;

    auto function_res = name == "f" ?
        std::async(std::launch::async, &f_function, x) :   
        std::async(std::launch::async, &g_function, x);    

    auto res = function_res.get();

    res.sig = name == "f" ? RESPONSE_SIG_F : RESPONSE_SIG_G;
    res.iterno = iterno;

    ::sendto(sockfd, &res, sizeof(res), 0, (const struct sockaddr *) &servaddr,  sizeof(servaddr));

    ::close(sockfd);
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        PROCESS_NAME.assign(argv[0], 17);
        std::cout << "Running Manager..." << std::endl;
        run_manager();
    }
    else if (argc == 4)
    {
        std::string name (argv[1]);
        auto x      = std::atof(argv[2]);
        auto iterno = std::atof(argv[3]);

        std::cout << "Running Client... " << name << std::endl;

        run_client(name, x, iterno);
    }
    else
        std::cerr << "Argument expected" << std::endl;
    
    return 0;
}