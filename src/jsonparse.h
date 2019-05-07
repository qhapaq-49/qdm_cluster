#ifndef _INC_JSONPARSE_H_
#define _INC_JSONPARSE_H_

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <string>
#include "agent.h"

using namespace std;

namespace jsonparse{
  vector<string> json2commands(const string fname);
  void agent2json(const string fname, agent &a);
  void savemasterjson(const string fname, agentmaster &master);
}
#endif
