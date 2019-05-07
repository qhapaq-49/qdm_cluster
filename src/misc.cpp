#include <string>
using namespace std;

void replace(string &str, const string from, const string to){
  string::size_type pos = str.find(from);
  while(pos != string::npos){
    str.replace(pos, from.size(), to);
    pos = str.find(from, pos + to.size());
  }
}

string str;
string getstr(){
  char c;
  std::string tstr = "";
  c = getchar();
  if(c != EOF){
    str = str + c;
  }
  if(str.find("\n") != std::string::npos){
    tstr = str;
    replace(tstr, "\n","");
    str = "";
  }
  return tstr;
}
