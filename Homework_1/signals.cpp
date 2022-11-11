#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  cout << "smash: got ctrl-Z" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if (smash.fg_job_pid != SmallShell::NO_CURRENT_FG_JOB)
  {
    smash.curFGJtoJobsList();
    cout << "smash: process "+ to_string(smash.fg_job_pid) +" was stopped" << endl;
  }
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if (smash.fg_job_pid != SmallShell::NO_CURRENT_FG_JOB)
  {
    smash.curFGJterminate();
    cout << "smash: process "+ to_string(smash.fg_job_pid) +" was killed" << endl;
  }
}

void alarmHandler(int sig_num) {
  SmallShell& smash = SmallShell::getInstance();
  if(smash.times_to_finish.empty())
    throw SmashException("alarmHandler: expected to have atleast one proccess in 'times to finish'.");
  pair<time_t, pair<int, string>> min_proccess = *(smash.times_to_finish.begin());
  
  //note that min_proccess.first is expected to be very close to the current time of running this code.
  //if the son proccess is still alive when alarm is called, send SIGKILL to it, and print timeout message. 
  time_t time_key = min_proccess.first;
  int proccess_pid = min_proccess.second.first;
  const string& cmd = min_proccess.second.second;
  if(smash.isStillAlive(proccess_pid))
  {  
    cout << "smash: got an alarm" << endl;
    //send SIGKILL:
    DO_SYS(kill(proccess_pid, SIGKILL), -1, kill);
    cout << "smash: " + cmd + " timed out!" << endl;
    //remove the proccess from 'times to finish':
    smash.times_to_finish.erase(time_key);
  }
}

