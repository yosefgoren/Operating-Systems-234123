#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <limits.h>
#include <set>
#include <map>
#include <array>
#include <assert.h>  
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <exception>
#include <unistd.h>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>

#ifdef  ALLOW_DBPRINTS
#define PRINT_PID std::cout << "the process with pid is: " + getpid() << std::endl
#else
#define PRINT_PID
#endif

#ifdef ALLOW_PRINT_FUN_NAMES
#define YELL(name) std::cout << std::string(name) + std::string(" function was called, from PID: " + std::to_string(getpid())) << std::endl
#else
#define YELL(name)//do nothing
#endif

#ifdef ALLOW_DBPRINTS
#define DBPRINT(name) std::cout << std::string(name) << std::endl
#else
#define DBPRINT(name)//do nothing
#endif

#define SYS_FAIL( cond, name) do {                                                    \
    if (cond) {                                                                       \
        throw SyscallFailedException(std::string(#name) + std::string(" failed"));    \
    }                                                                                 \
} while( 0 )
#define DO_SYS( syscall, fail_val, name ) SYS_FAIL( syscall == fail_val, name)
#define DO_SYS_RES( syscall, fail_val, name , res_target) SYS_FAIL((res_target = syscall) == fail_val, name)

#if DNDEBUG
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
 public:
  int time_out;
  const static int NO_TIME_LIM = -1;
  Command(int time_out);
  virtual ~Command() {}
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(int time_out);
  virtual ~BuiltInCommand() {}
};

class JobsList;
class ExternalCommand : public Command {
  std::string cmd_line;
  
 public:
  class CStringArr
  {
    char** args;
    int num_cstrings;
   public:
    /**
    * The resulting object allows returning an array of char* (aka cstring).
    * The Array of char* will also end with a NULL, it's length is one more than that of the vector.
    */
    CStringArr(const std::vector<std::string>& strings);
    ~CStringArr();
    char** getStringArr();
  };
  ExternalCommand(const std::string& cmd_line, int time_out);
  virtual ~ExternalCommand() = default;
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
  pid_t pid;
 public:
  ShowPidCommand(int time_out);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(int time_out);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  bool kill_all_jobs;
  JobsList* jobs_ref;
  bool* exit_flag_ref;
 public:
  QuitCommand(std::vector<std::string> args, int time_out, JobsList* jobs_ref, bool* exit_flag_ref);
  virtual ~QuitCommand() {}
  void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
  std::string* prompt_message_ref;
  std::string new_prompt_message;
 public:
  ChangePromptCommand(std::vector<std::string> args, int time_out, std::string* prompt_message_ref);
  virtual ~ChangePromptCommand() {}
  void execute() override;
};

class JobsCommand : public BuiltInCommand {
 JobsList* jobs_ref;
 public:
  JobsCommand(int time_out, JobsList* jobs_ref);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 JobsList* jobs_ref;
 unsigned long signum;
 unsigned long jobid;
 public:
  KillCommand(std::vector<std::string> args, int time_out, JobsList* jobs_ref);
  virtual ~KillCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
  bool do_nothing_flag;
  std::string& prev_cwd_ref;
  std::string new_pwd;
 public:
  ChangeDirCommand(const std::vector<std::string>& args, int time_out, std::string& lastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class CatCommand : public BuiltInCommand {
  std::string resulting_print;
 public:
  CatCommand(std::vector<std::string> args, int time_out);
  virtual ~CatCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList* jobs_ref;
 int job_id;
 public:
  ForegroundCommand(std::vector<std::string> args, int time_out, JobsList* jobs_ref);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 JobsList* jobs_ref;
 int job_id;
 public:
  BackgroundCommand(std::vector<std::string> args, int time_out, JobsList* jobs_ref);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class PipeErrorToWrite : public Command
{  
  std::vector<std::string> tokens1;
  std::vector<std::string> tokens2;
 public:
  PipeErrorToWrite(std::vector<std::string> tokens1, std::vector<std::string> tokens2, int time_out);
  virtual ~PipeErrorToWrite() {}
  void execute() override;
};

class PipeStdOToWrite : public Command
{  
  std::vector<std::string> tokens1;
  std::vector<std::string> tokens2;
 public:
  PipeStdOToWrite(std::vector<std::string> tokens1, std::vector<std::string> tokens2, int time_out);
  virtual ~PipeStdOToWrite() {}
  void execute() override;
};

class RedirectionAppend : public Command
{  
  std::vector<std::string> tokens1;
  std::vector<std::string> tokens2;
 public:
  RedirectionAppend(std::vector<std::string> tokens1, std::vector<std::string> tokens2, int time_out);
  virtual ~RedirectionAppend() {}
  void execute() override;
};

class RedirectionOverride : public Command
{  
  std::vector<std::string> tokens1;
  std::vector<std::string> tokens2;
 public:
  RedirectionOverride(std::vector<std::string> tokens1, std::vector<std::string> tokens2, int time_out);
  virtual ~RedirectionOverride() {}
  void execute() override;
};

//@JobsList

enum JobEntryStatus
{
  RUNNING,
  STOPPED
};

class JobsList {
  class JobEntry {
    JobEntry() = delete;
   public:
    int job_id; 
    int pid;
    std::string command;
    time_t time_start_running;  // should be updated on init or whenever the job is back from stopped status
    JobEntryStatus status;

    JobEntry(int job_id, int pid, bool is_stopped, const std::string& command/* , int time_to_live = NO_TIME_LIM */);
    void terminate();
    void sendSignal(int signal);
    void stop();
    void resume();
    int getElapsedTime() const;
  }; 
  
  std::map<int, JobEntry> jobs;
  std::vector<int> stopped_jobs_ids;
 
 public:
  JobsList();
  ~JobsList();
  void addJob(int pid, std::string command, bool is_stopped);
  void trackStoppedJob(int jid);
  void untrackStoppedJob(int jid); // doesn't change the vector if the pid doesn't exist
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId = nullptr);
  JobEntry *getLastStoppedJob(int *jobId = nullptr);
  void waitForJob(int jid);
  bool empty();
  bool stoppedJobExists();
  bool isInJobsList(int pid);
  int numOfJobs() const;
};

//@SmallShell

class SmallShell {
 private:
  std::string prompt_message;
  std::string prev_cwd; 
  bool exit_program_flag;
  SmallShell();
  ~SmallShell() = default;
 public:
  static int smash_pid;
  const static int NO_CURRENT_FG_JOB = -1;
  int fg_job_pid;
  int fg_job_jid;
  std::string fg_job_command;
  JobsList jobs_list;
  int stdin_fd_backup;

  /**
   * The key is the expected time for a job to end, and the value is a pair:
   *  the first of the pair is the pid, and the second is the cmd (command line) that created the command.
   * jobs that do not have a time limit (timeout) are not in the map.
   */
  std::map<time_t, std::pair<int, std::string>> times_to_finish;
  /**
   * given the pid of a (potentialy) son proccess, returns whether or not
   * that proccess is currently in the jobs list or in the fg.
   */
  bool isStillAlive(int son_pid);

  static const std::string NO_PREV_WORKING_DIR;
  static const std::string default_prompt_message;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  void executeCommand(const char* cmd_line);
  bool exitProgram() const;
  const std::string& getPromptMessage();
  const std::string& getPrevCwd() const { return prev_cwd; };
  void setPrevCwd(const std::string& prev_cwd) { this->prev_cwd = prev_cwd; }
  void curFGJtoJobsList();
  void curFGJterminate();
};

//@SmashException
class SmashException : public std::exception
{
protected:
    std::string error_message;
public:
    explicit SmashException(const std::string& error_message = std::string("An unexpected error has occurred"));
    virtual ~SmashException();
    virtual const char* what() const noexcept override;
};    

class SyscallFailedException : public SmashException 
{
public:
    SyscallFailedException(const std::string& error_message);
    virtual ~SyscallFailedException();
};

//@SmashUtil
namespace SmashUtil
{
    void insertSpaces(std::string& str, std::set<size_t> positions);
    std::vector<std::string> parseCommand(std::string line);
    std::vector<std::string> parse(std::string line, char parser = ' ');
    const std::string WHITESPACE = " \n\r\t\f\v";
    std::string _ltrim(const std::string& s);
    std::string _trim(const std::string& s);
    std::string _rtrim(const std::string& s);
    int _parseCommandLine(const char* cmd_line, char** args);
    bool _isBackgroundComamnd(const char* cmd_line);
    std::string removeBackgroundSign(std::string cmd_line);
    void _removeBackgroundSign(char* cmd_line);
    int our_stoi(const std::string&);
    time_t getTimeSince(time_t start_time);

    void splitArgsList(std::vector<std::string> args, int index_of_splitter,
        std::vector<std::string>* target1, std::vector<std::string>* target2);
    bool argsAreTextFile(std::vector<std::string> args);
}

#endif //SMASH_COMMAND_H_

