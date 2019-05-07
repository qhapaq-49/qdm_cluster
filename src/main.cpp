#include "agent.h"
#include "logger.h"

using namespace std;

int main(){
  string fileconfig = "config.txt";
  // cout<<"Sokhandan : load config file. filename = "<<fileconfig<<endl;
  ifstream ifs(fileconfig);
  start_logger(true);
  agentmaster master;
  master.init(ifs);
  ifs.close();
  // cout<<"main loop start"<<endl;
  usi::loop(master);
}
