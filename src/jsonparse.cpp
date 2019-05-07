#include "json11.h"
#include "jsonparse.h"
#include "misc.h"
#include <fstream>
using namespace std;

vector<string> jsonparse::json2commands(const string fname){
  vector<string> output;
  ifstream ifs;
  ifs.open(fname);
  string jsonstr="";
  string str;
  while(getline(ifs,str)){
    jsonstr += str;
  }

  string err;
  auto json = json11::Json::parse(jsonstr,  err);
  for (auto &k : json["command"].array_items()) {
    string cmd = k.dump();
    replace(cmd, "\"","");
    cout<<cmd<<endl;
    output.push_back(cmd);
  }
  return output;
}

void jsonparse::savemasterjson(const string fname, agentmaster &master){
   string outtmp = "{    \"mode\" : \"go\",    \"outdir\" : \"_OUTDIR_\",    \"position\" : \"_POS_\",    \"agent_num\" : _ANUM_,    _AGENTS_   }";
   string agenttmp =
     "\"agent_AID_\" : {	\"id\" : _AID_,	\"name\" : \"_NAME_\",	\"status\" : \"idle\"    }";
   replace(outtmp, "_OUTDIR_", master.jsondir);
   replace(outtmp, "_POS_", master.posstr);
   replace(outtmp, "_ANUM_", to_string(master.agents.size()));
   string agentstrsum = "";
   for(int i=0;i<master.agents.size();++i){
     string agentstr = agenttmp;
     replace(agentstr, "_AID_", to_string(i));
     replace(agentstr, "_NAME_", master.agents[i].agentname);
     agentstrsum += agentstr;
     if(i!=master.agents.size()-1){
       agentstrsum += ",";
     }
   }
   string err;
   replace(outtmp, "_AGENTS_", agentstrsum);
   cout<<outtmp<<endl;
   auto json = json11::Json::parse(outtmp, err);
   ofstream ofs;
   ofs.open(fname);
   ofs<<json.dump()<<endl;
}

void jsonparse::agent2json(const string fname, agent &a){
  string out = "{ \"log\" : [";
  vector<string> log = a.proc.getLines();
  for(int i=0;i<log.size();++i){  
    out += "\"" + log[i] + "\"" ;
    if(i!=log.size()-1){
      out += ",";
    }
  }
  out += "], \"id\" : " + to_string(a.id) + "}";
  //cout<<out<<endl;
  string err;
  auto json = json11::Json::parse(out, err);
  ofstream ofs;
  ofs.open(fname);
  ofs<<json.dump()<<endl;
}
