#ifndef _INC_PROCESS_H_
#define _INC_PROCESS_H_

#include <cstdio>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sstream>
#include <iostream>
#include <future>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

// Processクラスは技巧の写経をベースに細かい調整をしている

class Process{
 public:
  int StartProcess(const char* file, char* const argv[]);

  void addLine(const string str);
  void init_nonblock();
  bool updateLine();
  
  template<typename... Args>
    void Printf(const char* format, const Args&... args) {
    fprintf(stream_to_child_, format, args...);
  }
  
  void PrintLine(const char* str) {
    fprintf(stream_to_child_, "%s\n", str);
  }
  
  
  pid_t process_id() const {
    return process_id_;
  }

  bool isTimeout; // getLineで一定時間以上文字列を返さなかった奴は死んだとみなす
  string procName; //processの名前
  string getLine();
  vector<string> getLines();
  void clearLines();
 private:
  /** 外部プロセスのプロセスID */
  pid_t process_id_;

  string sandline;
  vector<string> lines;
  FILE* stream_to_child_;
  
  FILE* stream_from_child_;

};


extern mutex mutex_;

namespace processfunc{
  void boot(Process &proc, const bool isssh, const string sshname, const string sshdir, const string binname);
  template<typename... Args>
    void SendCommand(Process &process ,const char* format, const Args&... args) {
    if(process.isTimeout){
      return;
    }
    unique_lock<mutex> lock(mutex_);
    process.Printf(format, args...);
    process.Printf("\n");
    
  }
}


#endif
