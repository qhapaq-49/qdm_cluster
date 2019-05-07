#include "pvparse.h"

using namespace std;

packed_pv pvparse::pvparse(string str){
  istringstream is(str);
  string token;
  packed_pv pv;
  bool pvmode = false;
  while(true){
    token = "";
    is >> token;
    if (token == ""){
      break;
    }
    if (token =="depth"){
      is >> pv.depth;
      pvmode =false;
    }else if(token=="multipv"){
      is >> pv.multipv;
      pv.multipv -= 1;
    }else if (token=="score"){
      is >> token;
      pvmode =false;
      if(token=="cp"){
	is >> pv.eval;
      }else if(token=="Mate"){
	is >> pv.eval;
	// is it correct?
	if (pv.eval<0){
	  pv.eval -= 30000;
	}else{
	  pv.eval += 30000;
	}
      }
    }else if(token=="nodes"){
      is >> pv.nodes;
    }else if(token=="pv"){
      pvmode = true;
      is >> pv.pv;
    }else{
      /*
      if (pvmode){
	pv.pv += token+" ";
      }
      */
    }
  }
  return pv;
}

std::ostream& operator<<(std::ostream& os, const packed_pv &p){
  os<<"multipv, "<<p.multipv<< ", pv, "<<p.pv<<", eval," << p.eval << ", depth," << p.depth<<", nodes, "<<p.nodes;
  return os;
}
