#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    if(signal(SIGALRM, alarmHandler)==SIG_ERR) {
        perror("smash error: failed to set alarm handler");        
    }
    
    SmallShell& smash = SmallShell::getInstance();
    DBPRINT("main: starting command eval loop.");
    
    try
    {
        DO_SYS_RES(getpid(), -1, getpid, SmallShell::smash_pid);
    }
    catch(const SyscallFailedException& e)
    {
        perror(e.what());
    }

    
    while(!smash.exitProgram()) {
        #ifdef ALLOW_DBPRINTS
            std::cout << getpid() << ": ";
        #endif
        std::cout << smash.getPromptMessage() + "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        
        try
        {
            smash.executeCommand(cmd_line.c_str());
        }
        catch (const SyscallFailedException& e)
        {
            perror(e.what());
        }
        catch (const SmashException& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
    return 0;
}