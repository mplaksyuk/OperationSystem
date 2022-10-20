#include <iostream>
#include <csignal>
#include <thread>
#include <string>

int my_signal = 0;

class InProcess
{
public:
    InProcess() { IN_PROCESS = true; }

    ~InProcess() { IN_PROCESS = false; }

    static void handler(int s)
    {
        if (!IN_PROCESS)
            my_signal = s;
        else
            exit(1);
    }

    static bool IN_PROCESS;
    static bool STOP_PROCESS;
    static bool CONTINUE_PROCESS;
    static bool PROMPT;
};


void cancelationDialog() {

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = InProcess::handler;

    sigemptyset(&sigIntHandler.sa_mask);

    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    while(true)
    {
        if(my_signal != 0)
        {
            InProcess busy;

            InProcess::PROMPT = false;

            std::string ans;
            bool inputReceived = false;
            time_t startTime = time(NULL);
            time_t waitTime = 10;

            std::cout << std::endl << "Do u want to continue? (yes/no) or (y/n) " << std::endl << "wait for answer " << waitTime << " seconds" << std::endl;

            // spawn a concurrent thread that waits for input from std::cin
            std::thread t1([&]() {
                std::cout << "ans: ";
                std::cin >> ans;
                inputReceived = true;
            });
            t1.detach();

            // check the inputReceived flag once every 50ms for 10 seconds
            while (time(NULL) < startTime + waitTime && !inputReceived) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            
            if (inputReceived) {
                if(ans == "yes" || ans == "y")
                    InProcess::CONTINUE_PROCESS = true;
                else if (ans == "no" || ans == "n")
                    InProcess::STOP_PROCESS = true;
            }
            else 
                std::cout << "Answer timeout!" << std::endl;

            my_signal = 0;
        }
    };
}