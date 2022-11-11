#include <iostream>
#include <string>
#include <limits.h>
#include <limits>
#include "../SmashException.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <array>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

using namespace std;

int main(){
  cout << "entered main" << endl;
  

  int pipeio[2];
  pipe(pipeio);

  bool isBG = false;

  if(fork() == 0) {
    dup2(pipeio[0], 0);
    close(pipeio[1]);
    string s;
    
    while(cin >> s)
    {
      cout << "son: "+s << endl;
      if(s == "end")
        exit(0);
    }
  } else {
    close(pipeio[0]);
    string s;
    while(cin >> s)
    {
      if(!isBG)
      {
        write(pipeio[1], cin.rdbuf(), PIPE_BUF);
        wait(nullptr);
      }
      else if (s == "c")
        isBG = false;
    }
  }

  cout << "exiting main" << endl;
  return 0;
}
