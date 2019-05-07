#ifndef _INC_AGENT_H_
#define _INC_AGENT_H_

#include "process.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <string>
#include <queue>
#include <time.h> 

using namespace std;

enum agent_status{
  agent_boot,
  agent_idle,
  agent_cmd,
  agent_go,
  agent_ponder,
  agent_dead,
};

struct agent{
  agent(){
    status = agent_dead;
    posstr = "undefined";
  }
  int id;
  Process proc;
  agent_status status;
  bool isssh;
  string sshname;
  string sshdir;
  string binname;
  string agentname;
  string posstr;
  vector<string> params;
  void SendCommand(const string str);
  bool updateLine();
};

std::ostream& operator<<(std::ostream& os, const agent &c);
std::ostream& operator<<(std::ostream& os, const agent_status &status);

struct agentmaster{
  agentmaster(){
    gid = -1;
    rid = -1;
    diststr = "go depth 8";
    idstr = "QDM";
    author = "Passione Shogi Team";
  }
  vector<agent> agents;
  vector<int> ponder_nums;
  string idstr;
  string author;
  string posstr;
  string diststr;
  int gid;
  int rid;
  void init(ifstream &ifs);
  clock_t start_time;
  string prev_go_cmd;
  long int btime;
  long int wtime;
};

namespace usi{
  void loop(agentmaster &master);
}


#endif
