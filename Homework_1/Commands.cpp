#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include <map>
#include "Commands.h"
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <iostream>
#include <map>

using namespace std;
using namespace SmashUtil;

static void redirectAux(vector<string> tokens1, vector<string> tokens2,int flag);
static void pipeAux(vector<string> tokens1, vector<string> tokens2,int flag);
static string tokensToString(const vector<string>& tokens);
static void closePipe(int* pipe_io_ref);

Command::Command(int time_out) : time_out(time_out){
  //YELL("Command c'tor");
  }

BuiltInCommand::BuiltInCommand(int time_out):Command(time_out){
  //YELL("BuiltInCommand c'tor");
  }

ExternalCommand::CStringArr::CStringArr(const vector<string>& strings)
{
  args = new char*[strings.size()+1];
  int index = 0;
  for(const std::string& s : strings)
  {
    args[index] = new char[s.length()+1];
      
    strcpy(args[index], s.c_str());
    index++;
  }
  args[index] = NULL;
  num_cstrings = index+1;
}
ExternalCommand::CStringArr::~CStringArr()
{
  for(int i = 0; i < num_cstrings+1; ++i)
    delete[] args[i];
  delete[] args;
}
char** ExternalCommand::CStringArr::getStringArr()
{
  return args;
}

ExternalCommand::ExternalCommand(const string& cmd_line, int time_out):
  Command(time_out), cmd_line(cmd_line){
    //YELL("ExternalCommand c'tor");
    }

void ExternalCommand::execute(){
  YELL("ExternalCommand execute");
  
  bool background_command = _isBackgroundComamnd(cmd_line.c_str());
  SmallShell& smash = SmallShell::getInstance();

  int fork_res;
  DO_SYS_RES(fork(), -1, fork, fork_res);
  if(fork_res == 0)//in this case, the program is the son.
  {
    DO_SYS(setpgrp(), -1, setpgrp);
    try{
      ExternalCommand::CStringArr execv_args_aux = ExternalCommand::CStringArr(vector<string>({
        "/bin/bash",
        "-c",
        removeBackgroundSign(cmd_line)
      }));
      
      char** execv_args = execv_args_aux.getStringArr(); 
      DO_SYS(execv(execv_args[0], execv_args), -1, execv);
      
      //debug, should only get here if execv returned somehow
      PRINT_PID;
      DBPRINT("returned from execv");
      exit(1);
    }
    catch (const SyscallFailedException& e)
    {
      perror(e.what());
    }
    catch (const SmashException& e)
    {
      std::cerr << e.what() << std::endl;
    }
    catch (...){}
    exit(1);
  }
  else//in this case, the program is the father, and 'fork_res' is also the son pid.
  {
    DBPRINT("ExternalCommand execute: just forked. son is: "+to_string(fork_res));
    //check if there is timeout, if there is, add son pid to timeout list:
    if(time_out != NO_TIME_LIM)
    {
      //insert the proccess id and appropriate time to timeout proccesses list:
      time_t time_to_finish;
      DO_SYS(time(&time_to_finish), -1, time);
      time_to_finish += time_out;
      smash.times_to_finish.insert(pair<time_t, pair<int, string>>(time_to_finish, pair<int, string>(fork_res, cmd_line)));

      //set the alarm for killing the proccess/removing it from the list of timeout proccesses:
      alarm(time_out);//no error val can be returned from 'alarm()', so no need to catch that case.
    }

    if(background_command)//in this case, add the son to jobs list:
    {
      smash.jobs_list.addJob(fork_res, cmd_line, false);
    }
    else//in this case, the some in running in fg, and father should use waitpid on it:
    {
      int status = -1;
      smash.fg_job_pid = fork_res;
      smash.fg_job_command = cmd_line;
      DBPRINT("ExternalCommand execute: about to wait for son, pid: "+to_string(fork_res));
      DO_SYS(waitpid(fork_res, &status, WUNTRACED), -1, waitpid);
      smash.fg_job_pid = SmallShell::NO_CURRENT_FG_JOB;
      smash.fg_job_command = "";
      DBPRINT("ExternalCommand execute: waiting for son finished. resulting status: "+to_string(status));
    }
  }
}

ShowPidCommand::ShowPidCommand(int time_out) : BuiltInCommand(time_out)
{
  //YELL("ShowPidCommand c'tor");
}
void ShowPidCommand::execute()
{
  YELL("ShowPidCommand execute");
  cout << "smash pid is " << SmallShell::getInstance().smash_pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(int time_out) : BuiltInCommand(time_out) {
  //YELL("GetCurrDirCommand c'tor");
  }

void GetCurrDirCommand::execute()
{
  YELL("GetCurrDirCommand execute");
  char cwd[PATH_MAX];
  DO_SYS(getcwd(cwd, sizeof(cwd)), NULL, getcwd);
  cout << string(cwd) << endl;
}

QuitCommand::QuitCommand(vector<string> args, int time_out, JobsList* jobs_ref, bool* exit_flag_ref):
  BuiltInCommand(time_out), kill_all_jobs(!args.empty() && (args[0].compare("kill") == 0)),
  jobs_ref(jobs_ref), exit_flag_ref(exit_flag_ref){
  //YELL("QuitCommand c'tor");
}
void QuitCommand::execute()
{
  YELL("QuitCommand execute");
  //jobs_ref->removeFinishedJobs();
  if(kill_all_jobs)
  {
    cout << "smash: sending SIGKILL signal to "+ to_string(jobs_ref->numOfJobs()) +" jobs:" << endl;
    jobs_ref->killAllJobs();
  }
  *exit_flag_ref = true;
}

ChangePromptCommand::ChangePromptCommand(std::vector<std::string> args, int time_out, string* prompt_message_ref)
  :BuiltInCommand(time_out), prompt_message_ref(prompt_message_ref)
{
  //YELL("ChangePromptCommand c'tor");
  new_prompt_message = (args.empty())? SmallShell::default_prompt_message : args[0];
}
void ChangePromptCommand::execute()
{
  YELL("ChangePromptCommand execute");
  *prompt_message_ref = new_prompt_message;
}

JobsCommand::JobsCommand(int time_out, JobsList* jobs_ref):
  BuiltInCommand(time_out), jobs_ref(jobs_ref){
  //YELL("JobsCommand c'tor");
  }
void JobsCommand::execute(){
  YELL("JobsCommand execute");
  //jobs_ref->removeFinishedJobs();
  jobs_ref->printJobsList();
}

KillCommand::KillCommand(vector<string> args, int time_out, JobsList* jobs_ref):
  BuiltInCommand(time_out), jobs_ref(jobs_ref)
{
  YELL("KillCommand c'tor");
  if (args.size() != 2 || (args[0].find_first_of('-') != 0) || (args[0].empty()) || (args[1].empty()))
    throw SmashException("kill: invalid arguments");
  args[0].erase(args[0].begin());
  try {
    signum = our_stoi(args[0]);
  }    
  catch (const std::exception& e) {
    throw SmashException("kill: invalid arguments");
  }
  try {
    jobid = our_stoi(args[1]);
  }
  catch (const std::exception& e) {
    if (strcmp(e.what(), "negative input is not allowed") == 0)
      throw SmashException(string("kill: job-id ") + args[1] + string(" does not exist"));
    throw SmashException("kill: invalid arguments");
  }
}
void KillCommand::execute()
{
  YELL("KillCommand execute");
  try {
    jobs_ref->getJobById(jobid)->sendSignal(signum);
  }
  catch (const std::out_of_range& e) {
    throw SmashException("kill: job-id " + to_string(jobid) + " does not exist");
  }
  cout << "signal number " << signum << " was sent to pid " 
       << jobs_ref->getJobById(jobid)->pid << endl;
  //jobs_ref->removeJobById(jobid);
  //TODO:
}

ChangeDirCommand::ChangeDirCommand(const vector<string>& args, int time_out, string& prev_cwd_ref)
  : BuiltInCommand(time_out), do_nothing_flag(false), prev_cwd_ref(prev_cwd_ref)
{
  YELL("ChangeDirCommand c'tor");
  if(args.size() == 0)
  {
    do_nothing_flag = true;
    return;
  }
  else if (args.size() > 1) {
    throw SmashException("cd: too many arguments");
  }
  else if ((args[0].compare("-") == 0) && prev_cwd_ref.compare(SmallShell::NO_PREV_WORKING_DIR) == 0) {
    throw SmashException("cd: OLDPWD not set"); 
  }
  new_pwd = ((args[0].compare("-") == 0)? prev_cwd_ref : args[0]);
}

void ChangeDirCommand::execute(){
  YELL("ChangeDirCommand execute");
  if(do_nothing_flag)
    return;
  char cwd[PATH_MAX];
  DO_SYS(getcwd(cwd, sizeof(cwd)), NULL, getcwd); 
  DO_SYS(chdir(new_pwd.c_str()) , -1, chdir);
  prev_cwd_ref = string(cwd);
}

CatCommand::CatCommand(vector<string> args, int time_out):
  BuiltInCommand(time_out), resulting_print(""){
  YELL("CatCommand c'tor");
  for(string file_name : args)
  {
    int fd = -1;
    DO_SYS_RES(open(file_name.c_str(), O_RDONLY), -1, open, fd);
    
    char next_char;
    ssize_t read_res;
    do{
      DO_SYS_RES(read(fd, &next_char, 1), -1, read, read_res);
      if(read_res == 1)
        resulting_print.push_back(next_char);
      else
        break;
    }while(true);

    DO_SYS(close(fd), -1, close);
  }
}
void CatCommand::execute(){
  YELL("CatCommand execute");
  cout << resulting_print;
}

ForegroundCommand::ForegroundCommand(std::vector<std::string> args, int time_out, JobsList* jobs_ref):
  BuiltInCommand(time_out), jobs_ref(jobs_ref){
  YELL("ForegroundCommand c'tor");
  if (jobs_ref->empty())
    throw SmashException("fg: jobs list is empty");
  if (args.size() > 1)
    throw SmashException("fg: invalid arguments");
  else if (!args.empty()) {
    try{
      job_id = our_stoi(args[0]);
    }
    catch(...){
      throw SmashException("fg: invalid arguments");
    }
  }
  else{
    jobs_ref->getLastJob(&job_id);
  }
}
void ForegroundCommand::execute(){
  YELL("ForegroundCommand execute");
  try{
    jobs_ref->getJobById(job_id);
  }
  catch(...){
     throw SmashException("fg: job-id " + to_string(job_id) + " does not exist");
  }

  SmallShell& smash = SmallShell::getInstance();
  smash.fg_job_pid = jobs_ref->getJobById(job_id)->pid;
  smash.fg_job_command = jobs_ref->getJobById(job_id)->command;  
  jobs_ref->waitForJob(job_id);
  smash.fg_job_pid = SmallShell::NO_CURRENT_FG_JOB;

}

BackgroundCommand::BackgroundCommand(vector<string> args, int time_out, JobsList* jobs_ref):
  BuiltInCommand(time_out), jobs_ref(jobs_ref){
  YELL("BackgroundCommand c'tor");
  if (args.size() > 1)
    throw SmashException("bg: invalid arguments");
  else if (!args.empty()) {
    try{
      job_id = our_stoi(args[0]);
    }
    catch(...){
      throw SmashException("bg: invalid arguments");
    }
  }
  if (jobs_ref->empty())
  {
    if (args.size() == 1)
      throw SmashException("bg: job-id " + to_string(job_id) + " does not exist");
  }
  if (!(jobs_ref->stoppedJobExists()))  //covers the case when no jobs and args.size is 0
    throw SmashException("bg: there is no stopped jobs to resume");
  else if (args.empty())
    jobs_ref->getLastStoppedJob(&job_id);
  
}
void BackgroundCommand::execute(){
  YELL("BackgroundCommand execute");
  try{
    jobs_ref->getJobById(job_id);
  }
  catch(...){
     throw SmashException("bg: job-id " + to_string(job_id) + " does not exist");
  }

  if (jobs_ref->getJobById(job_id)->status == RUNNING)
    throw SmashException("bg: job-id " + to_string(job_id) + " is already running in the background");
    
  cout << jobs_ref->getJobById(job_id)->command << " : " << jobs_ref->getJobById(job_id)->pid << endl;
  jobs_ref->getJobById(job_id)->resume();
}

// PipeCommand::PipeCommand(vector<string> tokens, int time_out):
//   Command(time_out){
//   YELL("PipeCommand c'tor");
//   //TODO: LTI 
// }

PipeErrorToWrite::PipeErrorToWrite(vector<string> tokens1, vector<string> tokens2, int time_out):
  Command(time_out), tokens1(tokens1), tokens2(tokens2){YELL("PipeErrorToWrite c'tor");}
void PipeErrorToWrite::execute(){
  YELL("PipeErrorToWrite execute");
  pipeAux(tokens1, tokens2, 2);
}

PipeStdOToWrite::PipeStdOToWrite(vector<string> tokens1, vector<string> tokens2, int time_out):
  Command(time_out), tokens1(tokens1), tokens2(tokens2){YELL("PipeStdOToWrite c'tor");}
void PipeStdOToWrite::execute(){
  YELL("PipeStdOToWrite execute");
  pipeAux(tokens1, tokens2, 1);
}

RedirectionAppend::RedirectionAppend(vector<string> tokens1, vector<string> tokens2, int time_out):
  Command(time_out), tokens1(tokens1), tokens2(tokens2){YELL("RedirectionAppend c'tor");}
void RedirectionAppend::execute(){
  YELL("RedirectionAppend execute");
  int flag = O_APPEND;
  redirectAux(tokens1, tokens2, flag);
}

RedirectionOverride::RedirectionOverride(vector<string> tokens1, vector<string> tokens2, int time_out):
  Command(time_out), tokens1(tokens1), tokens2(tokens2){YELL("RedirectionOverride c'tor");}
void RedirectionOverride::execute()
{
  YELL("RedirectionOverride execute");
  int flag = O_TRUNC;
  redirectAux(tokens1, tokens2, flag);
}

void pipeAux(vector<string> tokens1, vector<string> tokens2, int write_fd)
{
  int pipe_io[2];
  DO_SYS(pipe(pipe_io), -1, pipe);
  
  SmallShell& smash = SmallShell::getInstance();
  
  int fork_res1 = -1;
  try{
    DO_SYS_RES(fork(), -1, fork, fork_res1);
    if(fork_res1 == 0)//case of first son:
    {
      DO_SYS(setpgrp(), -1, setpgrp);
      //redirect appropriate stream to pipe:
      DO_SYS(dup2(pipe_io[1], write_fd), -1, dup2);
      closePipe(pipe_io);
      
      //run the first command:
      string cmd1 = tokensToString(tokens1);
      //inside this command is another fork, so after finishing the executem we should end this son proccess.
      smash.executeCommand(cmd1.c_str());
      exit(0);
    }
  }
  catch (const SyscallFailedException& e)
  {
    perror(e.what());
  }
  catch (const SmashException& e)
  {
    cerr << e.what() << endl;
  }
  if(fork_res1 == 0)
    exit(0);

  int fork_res2 = -1;
  try{
    DO_SYS_RES(fork(), -1, fork, fork_res2);
    if(fork_res2 == 0)//case of second son:
    {
      DO_SYS(setpgrp(), -1, setpgrp);
      //redirect in to pipe
      DO_SYS(dup2(pipe_io[0], 0), -1, dup2);
      closePipe(pipe_io);
      
      //run the second command:
      string cmd2 = tokensToString(tokens2);
      smash.executeCommand(cmd2.c_str());
      DBPRINT("printAux: second son has returned from executeCommand.");
      exit(0);
    }
  }
  catch (const SyscallFailedException& e)
  {
    perror(e.what());
  }
  catch (const SmashException& e)
  {
    cerr << e.what() << endl;
  }
  catch (...)
  {
    cerr << "forked son from pipe: unknown exception was thrown." << endl;
  }
  if(fork_res2 == 0)
    exit(0);
  closePipe(pipe_io);

  DO_SYS(waitpid(fork_res1, nullptr, 0), -1, waitpid);
  DBPRINT("printAux: (father) finished waiting for first son. pid: "+to_string(fork_res1));
  DO_SYS(waitpid(fork_res2, nullptr, 0), -1, waitpid);
  DBPRINT("printAux: (father) finished waiting for second son. pid: "+to_string(fork_res2));
}

void redirectAux(vector<string> tokens1, vector<string> tokens2,int flag)
{
  SmallShell& smash = SmallShell::getInstance();

  string cmd1 = tokensToString(tokens1);

  int stdout_fd_backup = -1;
  DO_SYS_RES(dup(1),-1,dup, stdout_fd_backup);
  DO_SYS(close(1),-1, close);
  int text_fd = -1;
  DO_SYS_RES(open(tokens2[0].c_str(), O_RDWR | O_CREAT | flag, 0777), -1, open, text_fd);
  smash.executeCommand(cmd1.c_str()); //no need to try catch because smash will catch if this line throws
  //^^^^ maybe need to make sure child is exited to avoid running two smashes
  //DO_SYS(close(1),-1, close);
  DO_SYS(dup2(stdout_fd_backup, 1), -1, dup2);
}

string tokensToString(const vector<string>& tokens)
{
  string out;
  for (const string& s : tokens)
  {
    out += s + " ";
  }

  return out;
}

void closePipe(int* pipe_io_ref){
  DO_SYS(close(pipe_io_ref[0]), -1, close);
  DO_SYS(close(pipe_io_ref[1]), -1, close);
}

//@JobsList

JobsList::JobEntry::JobEntry(int job_id, int pid, bool is_stopped,const string& command/*, int time_to_live */)
      : job_id(job_id), pid(pid), command(command)//, time_to_live(time_to_live)
{
    status = is_stopped ? STOPPED : RUNNING;
    DO_SYS(time(&time_start_running), -1, time); // status doesn't matter here
}

void JobsList::JobEntry::sendSignal(int signal)
{
  YELL("JobEntry sendSignal");
  DO_SYS(kill(pid, signal), -1, kill);
}

void JobsList::JobEntry::stop()
{
  YELL("JobEntry stop");
  if (status == RUNNING)
  {
    sendSignal(SIGSTOP);
    status = STOPPED;
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list.trackStoppedJob(job_id);
  }
}

void JobsList::JobEntry::resume()
{
  if (status == STOPPED)
  {
    //DO_SYS(time(&time_start_running), -1, time);
    sendSignal(SIGCONT);
    status = RUNNING;
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list.untrackStoppedJob(pid);
  }
}

int JobsList::JobEntry::getElapsedTime() const
{
  return SmashUtil::getTimeSince(time_start_running);
}

JobsList::JobsList(){}
JobsList::~JobsList()
{
  killAllJobs();
}

void JobsList::addJob(int pid, string command, bool is_stopped)
{
  YELL("JobsList addJob");
  int next_jid;
  if(jobs.empty())
    next_jid = 1;
  else {
    int current_max_jid = jobs.rbegin()->first;
    next_jid = current_max_jid+1;
  }

  jobs.insert({next_jid, JobEntry(next_jid, pid, is_stopped, command/*, JobEntry::NO_TIME_LIM */)});
  //DBPRINT(to_string(jobs.size()));
}

void JobsList::trackStoppedJob(int jid)
{
  YELL("JobsList trackStoppedJob");
  stopped_jobs_ids.push_back(jid);
}

void JobsList::untrackStoppedJob(int jid)
// doesn't change the vector if the jid doesn't exist
{
  for (auto it = stopped_jobs_ids.begin(); it != stopped_jobs_ids.end(); ++it)
  {
    if (*it == jid)
    {
      stopped_jobs_ids.erase(it);
      return;
    }
  }
}
void JobsList::printJobsList() 
{
  YELL("JobsList printJobsList");
  string current_job_string;
  for (const auto& mapped_job : jobs) 
  {
    vector<string> parts =
    {
      "[",
      to_string(mapped_job.second.job_id),
      "] ",
      mapped_job.second.command,
      " : ",
      to_string(mapped_job.second.pid),
      " ",
      to_string(mapped_job.second.getElapsedTime()),
      " secs"
    };
    current_job_string = "";
    for(string s : parts)
      current_job_string += s;
    if (mapped_job.second.status == STOPPED) 
    {
      current_job_string += " (stopped)";
    }

    cout << current_job_string << endl;
  }
}

void JobsList::killAllJobs()
{
  vector<int> killed_job_jids;
  for(pair<const int, JobEntry>& pair : jobs){
    cout << pair.second.pid << ": " << pair.second.command << endl;
    pair.second.sendSignal(SIGKILL); //maybe use SIGTERM instead
    killed_job_jids.push_back(pair.second.job_id);
  }

  for (int jid : killed_job_jids)
    jobs.erase(jid);
}

void JobsList::removeFinishedJobs() 
{
  YELL("JobsList removeFinishedJobs");
  set<int> finished_job_jids = {};
  for(const auto& int_job_pair : jobs)
  {
    const JobEntry& job = int_job_pair.second;
    int status = -1;//don't chnage this or dire things will happends.
    
    DO_SYS(waitpid(job.pid, &status, WNOHANG), -1, waitpid);//From here the zombie should be gone.
    DBPRINT("removeFinishedJobs: job.pid: "+to_string(job.pid)+", WIFEXITED: "
    + to_string(WIFEXITED(status)) + ", WIFSIGNALED: " + to_string(WIFSIGNALED(status))
    + ", WTERMSIG:" + to_string(WTERMSIG(status)));
    
    if(WIFEXITED(status) || WIFSIGNALED(status))
      finished_job_jids.insert(job.job_id);
  }

  for(int jid : finished_job_jids)//now we only have to remove the job from the JobsList:
    jobs.erase(jid);
}

JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  return &(jobs.at(jobId));
}

void JobsList::removeJobById(int jobId)
{
  jobs.at(jobId).sendSignal(SIGKILL);
}


JobsList::JobEntry* JobsList::getLastJob(int* lastJobId)
{
  if (jobs.empty())
    throw std::logic_error("jobs is empty");
  if(lastJobId != nullptr) 
    *lastJobId = jobs.rbegin()->first;
  return &jobs.at(jobs.rbegin()->first);
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId)
{
  if (stopped_jobs_ids.empty())
    throw std::logic_error("jobs is empty");
  if(jobId != nullptr)
    *jobId = stopped_jobs_ids.back();
  DBPRINT("getLastStoppedJob: trying to return job with jid: "+to_string(stopped_jobs_ids.back()));
  return &jobs.at(stopped_jobs_ids.back());
}

bool JobsList::empty() 
{
  return jobs.empty();
}
bool JobsList::stoppedJobExists()
{
  return !(stopped_jobs_ids.empty());
}

void JobsList::waitForJob(int jid)
{
  YELL("JobsList waitForJob");

  int pid = getJobById(jid)->pid;
  SmallShell& smash = SmallShell::getInstance();
  smash.fg_job_pid = pid;
  DBPRINT("waitForJob: about to send SIGCONT to son: "+to_string(pid));
  cout << getJobById(jid)->command << " : " << pid << endl;
  getJobById(jid)->resume();

  int status;
  DBPRINT("waitForJob: about to wait to son-pid: "+to_string(pid));
  DO_SYS(waitpid(pid, &status, WUNTRACED), -1, waitpid);
  DBPRINT("waitForJob: done waiting on fg");
  // if (WIFEXITED(status) || WIFSIGNALED(status))
  //   jobs.erase(jid);
  removeFinishedJobs();
}

int JobsList::numOfJobs() const
{
  return jobs.size();
}

bool JobsList::isInJobsList(int pid){
  for(auto pair : jobs)
    if(pair.second.pid == pid)
      return true;
  return false;
}

//@SmashException

SmashException::SmashException(const string& error_message) : error_message("smash error: ")
{
    this->error_message += error_message;
}

SmashException::~SmashException(){}

const char* SmashException::what() const noexcept
{
    return error_message.c_str();
}

SyscallFailedException::SyscallFailedException(const std::string& error_message):
    SmashException(error_message) {}

SyscallFailedException::~SyscallFailedException(){}

//@SmallShell

int SmallShell::smash_pid = -1;

const string SmallShell::default_prompt_message = "smash";
const string SmallShell::NO_PREV_WORKING_DIR = "";

enum BuiltInCommandType
{
  CHPROMPT,
  SHOWPID,
  PWD,
  CD,
  JOBS,
  KILL,
  FG,
  BG,
  QUIT,
  CAT
};

enum ModefiedCommandType
{
  REDIRECTION_APPEND, // >>
  REDIRCION_OVERRIDE, // >
  PIPE_STD_TO_WRITE,  // |  redirects command1 stdout to its write channel and command2 stdin to its read channel.
  PIPE_ERROR_TO_WRITE // |& redirects command1 stderr to the pipe’s write channel and command2 stdin to the pipe’s read channel. 
};

const std::string& SmallShell::getPromptMessage()
{
  return prompt_message;
}

SmallShell::SmallShell()
  :prompt_message(SmallShell::default_prompt_message), prev_cwd(NO_PREV_WORKING_DIR), exit_program_flag(false),
    fg_job_pid(NO_CURRENT_FG_JOB), fg_job_command(""), jobs_list()
  {   
    DO_SYS_RES(dup(0), -1, dup, stdin_fd_backup);
  }

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line) {
	vector<string> tokens = parseCommand(string(cmd_line));
  
  //look for timeout modefier:
  int time_out = Command::NO_TIME_LIM;
  bool is_timeout = (!tokens.empty() && (tokens[0].compare("timeout") == 0));
  if(is_timeout)
  {
    tokens.erase(tokens.begin());
    if(!tokens.empty())
    {
      try
      {
        time_out = our_stoi(tokens[0]);//stoi() function converts a string to a cooresponding number.
      }
      catch(const std::exception& e)
      {
        throw SmashException("timeout: bad arguments");
      }
      
      tokens.erase(tokens.begin());
    }
  }

  //look for command modefiers:
  const map<string, ModefiedCommandType> mod_symbols = 
  {
    {"|&", PIPE_ERROR_TO_WRITE},
    {">>", REDIRECTION_APPEND},
    {"|", PIPE_STD_TO_WRITE},
    {">", REDIRCION_OVERRIDE}
  };
  int token_index = 0;
  for(string token : tokens)
  {
    for(std::pair<string, ModefiedCommandType> mod_symbol : mod_symbols)
      if(mod_symbol.first.compare(token) == 0)
      {
        vector<string> tokens1;
        vector<string> tokens2;
        splitArgsList(tokens, token_index, &tokens1, &tokens2);
        switch (mod_symbol.second)
        {
        case PIPE_ERROR_TO_WRITE:
          return new PipeErrorToWrite(tokens1, tokens2, time_out);
        case REDIRECTION_APPEND:
          return new RedirectionAppend(tokens1, tokens2, time_out);
        case PIPE_STD_TO_WRITE:
          return new PipeStdOToWrite(tokens1, tokens2, time_out);
        case REDIRCION_OVERRIDE:
          return new RedirectionOverride(tokens1, tokens2, time_out);
        default:
          break;
        }
      }
    token_index++;
  }
  //look for one of the built in keywords:
  string first_word = "";
  if(!tokens.empty())
    first_word = tokens[0];

  map<string, BuiltInCommandType> str_to_type = {
    {"chprompt", CHPROMPT},
    {"chprompt&", CHPROMPT},
    {"showpid", SHOWPID},
    {"showpid&", SHOWPID},
    {"pwd", PWD},
    {"pwd&", PWD},
    {"cd", CD},
    {"cd&", CD},
    {"jobs", JOBS},
    {"jobs&", JOBS},
    {"kill", KILL},
    {"kill&", KILL},
    {"fg", FG},
    {"fg&", FG},
    {"bg", BG},
    {"bg&", BG},
    {"quit", QUIT},
    {"quit&", QUIT},
    {"cat", CAT},
    {"cat&", CAT}
  };

  BuiltInCommandType COMMAND_TYPE;
  if(str_to_type.count(first_word) != 0) {
  //built in command:
    tokens.erase(tokens.begin());
    COMMAND_TYPE = str_to_type.at(first_word);
    switch (COMMAND_TYPE)
    {
    case CHPROMPT:
      return new ChangePromptCommand(tokens, is_timeout, &prompt_message);
    case SHOWPID:
      return new ShowPidCommand(is_timeout);
    case PWD:
      return new GetCurrDirCommand(time_out);
    case CD:
      return new ChangeDirCommand(tokens, is_timeout, prev_cwd);
    case JOBS:
      return new JobsCommand(is_timeout, &jobs_list);
    case KILL:
      return new KillCommand(tokens, is_timeout, &jobs_list);
    case FG:
      return new ForegroundCommand(tokens, is_timeout, &jobs_list);
    case BG:
      return new BackgroundCommand(tokens, is_timeout, &jobs_list);
    case QUIT:
      return new QuitCommand(tokens, is_timeout, &jobs_list, &exit_program_flag);
    case CAT:
      return new CatCommand(tokens, is_timeout);
    default:
      break;
    }
  }
  //(else) command is external:
  return new ExternalCommand(cmd_line, time_out);
}

void SmallShell::executeCommand(const char *cmd_line) {
  jobs_list.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  delete cmd;
}

bool SmallShell::exitProgram() const{
  return exit_program_flag;
}

void SmallShell::curFGJtoJobsList()
{
  YELL("SmallShell curFGJtoJobsList");
  if(fg_job_pid == NO_CURRENT_FG_JOB)
    throw SmashException("curFGJtoJobsList: expected to have an fg job.");
  if (jobs_list.empty() || jobs_list.getLastJob()->pid != fg_job_pid)
    jobs_list.addJob(fg_job_pid, fg_job_command, false);
  jobs_list.getLastJob()->stop();
  DBPRINT("curFGJtoJobsList: finished stopping job: "+to_string(fg_job_pid));
}

void SmallShell::curFGJterminate()
{
  DO_SYS(kill(fg_job_pid, SIGKILL), -1, kill);
  //int status = -1;
  //DO_SYS(waitpid(fg_job_pid, &status, WNOHANG), -1, waitpid);
  jobs_list.removeFinishedJobs();
}

bool SmallShell::isStillAlive(int son_pid)
{
  jobs_list.removeFinishedJobs();
  if(son_pid == fg_job_pid)
    return true;
  return jobs_list.isInJobsList(son_pid);
}

//@SmashUtil


static const vector<string> PIPE_OPERATORS =
{
  "|&",
  ">>",
  ">",
  "|"
};

void SmashUtil::insertSpaces(string& str, set<size_t> positions)
{  
  size_t offset = 0;
  for(auto position : positions)
  {
    size_t at_pos = position + offset++;
    string pre = str.substr(0, at_pos);
    string post = str.substr(at_pos, str.length());
    str = pre + " " + post;
  }
}

string withSpaces(string str, set<size_t> positions)
{
  insertSpaces(str, positions);
  return str;
}

vector<string> SmashUtil::parseCommand(string line)
{
  for(string op : PIPE_OPERATORS)
  {
    //check if operator can be found whithin the word, and if so - divide it:
    size_t position = line.find(op);
    if(position != string::npos)
    {
      insertSpaces(line, set<size_t>({position, position+op.length()}));
      break;
    }
  }
  return parse(line);
}

vector<string> SmashUtil::parse(string line, char delimiter)
{
    vector<string> tokens;
    
    while(!line.empty())
    {
        size_t token_start_pos = line.find_first_not_of(delimiter);
        if(token_start_pos == string::npos)
            return tokens;
        line = line.substr(token_start_pos, line.length());//crop delimiters from the front.
        
        string next_token = line.substr(0, line.find_first_of(delimiter));
        if(next_token != "") {tokens.push_back(next_token);}//insert the next token to the vector.
        
        size_t pos_after_token = line.find_first_of(delimiter);
        if(pos_after_token == string::npos)
            pos_after_token = line.length();
        line = line.substr(pos_after_token, line.length());//remove the next token from the line.
    }
    return tokens;
}

string SmashUtil::_ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string SmashUtil::_rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string SmashUtil::_trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int SmashUtil::_parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  //<cmd1>|<cmd2> -> <cmd1>, |, <cmd2>
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool SmashUtil::_isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

string SmashUtil::removeBackgroundSign(string cmd_line)
{
  //YELL("SmashUtil removeBackgroundSign");

  size_t position = cmd_line.find_last_of('&');
  if(position == string::npos)
    return cmd_line;

  vector<string> tokens = parse(withSpaces(cmd_line, set<size_t>({position})), ' ');
  if (tokens.back() != "&")
    return cmd_line;
  tokens.pop_back();
  
  string res = "";
  for(string token : tokens)
    res += token + " ";
  if (!res.empty())
    res.pop_back();
  //DBPRINT("removeBackgroundSign returning " + res);
  return res;
}

void SmashUtil::_removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

int SmashUtil::our_stoi(const std::string& s) 
{
  if(s.find_first_of('-') != std::string::npos){
    throw std::invalid_argument("negative input is not allowed");
  }
  return stoi(s);
}

time_t SmashUtil::getTimeSince(time_t start_time)
{
  time_t current_time;
  DO_SYS(time(&current_time), -1, time);
  return current_time - start_time;
}


void SmashUtil::splitArgsList(vector<string> args, int index_of_splitter, 
  vector<string>* target1, vector<string>* target2)
{
  int index = -1;
  for(const string& s : args)
  {
    ++index;
    if(index < index_of_splitter)
      target1->push_back(s);
    if(index == index_of_splitter)
      continue;
    if(index > index_of_splitter)
      target2->push_back(s);
  }
}

bool SmashUtil::argsAreTextFile(std::vector<std::string> args)
{
  if(args.size() != 1)
    return false;
  
  string word = args.back();
  size_t dot_index = word.find_last_of('.');
  if(dot_index == string::npos)
    return false;
  return word.substr(dot_index, word.length()) == ".txt";
}