#ifndef _INC_PVPARSE_H_
#define _INC_PVPARSE_H_

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <string>
using namespace std;

struct packed_pv{
  packed_pv(){
    multipv = 0;
    pv = "undefined";
  }
  int eval;
  string pv;
  int nodes;
  int nps;
  int time;
  int depth;
  int multipv;
};

std::ostream& operator<<(std::ostream& os, const packed_pv &p);

namespace pvparse{
  packed_pv pvparse(string str);
}

#endif
