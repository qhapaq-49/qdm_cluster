#include "agent.h"

std::ostream& operator<<(std::ostream& os, const agent &c){
  os << "agent : "<<c.agentname<< " status = " <<c.status<< ",isssh = "<<c.isssh <<", sshname ="<<c.sshname <<", sshdir = "<<c.sshdir <<", binname = "<<c.binname;
  os << ", params = ";
  for(auto p : c.params){
    os << p << ", ";
  }
  os<<c.posstr;
  return os;
}

std::ostream& operator<<(std::ostream& os, const agent_status &status){
  if(status == agent_idle){
    os<<"idle";
  }else if(status == agent_go){
    os<<"go";
  }else if(status == agent_dead){
    os<<"dead";
  }else if(status == agent_boot){
    os<<"boot";
  }else{
    os<<status <<" (undefined)";
  }
  return os;
}


void agentmaster::init(ifstream &ifs){
  string str;
  int id = -1;
  
  ponder_nums.resize(2);
  ponder_nums[0] = 0;
  ponder_nums[1] = 0;
  
  while(getline(ifs,str)){
    istringstream is(str);
    string token = "";
    is >> token;
    if(token=="#" || token == ""){
      continue;
    }
    if(token=="dist"){
      diststr = str.substr(4,str.length()-4);
    }else if(token=="id"){
      idstr = str.substr(2,str.length()-2);
      idstr += " feat QDM";
    }else if(token=="author"){
      author = str.substr(6,str.length()-6);
      author += " feat PST";
    }else if(token=="pondernum"){
      is >> ponder_nums[0] >> ponder_nums[1];
    }else if(token=="node"){
      id += 1;
      agents.resize(id+1);
      is >> agents[id].agentname;
      agents[id].id = id;
      string mode;
      is >> mode;
      if(mode == "local"){
	is >> agents[id].binname;
	agents[id].isssh = false;
      }else if(mode =="remote"){
	is >> agents[id].sshname >> agents[id].sshdir >>agents[id].binname;
	agents[id].isssh = true;
      }else if(mode =="generator"){
	if (gid == -1){
	  is >> agents[id].binname;
	  agents[id].isssh = false;
	  gid = id;
	}else{
	  cout<<"warning duplicated generator, ignored"<<endl;
	}
      }else{
	cout<<"warning undefined mode : "<<mode<<" ("<<str<<")" << endl;
      }
      
    }else{
      if(id >= 0){
	agents[id].params.push_back(str);
      }
    }
  }
  cout<<"diststr "<<diststr<<endl;
  for(auto c : agents){
    cout<<c<<endl;
  }
  
  for(auto &c: agents){
    c.status = agent_boot;
    processfunc::boot(c.proc, c.isssh, c.sshname, c.sshdir, c.binname);
    for(auto param :c.params){
      processfunc::SendCommand(c.proc, param.c_str());
    }
    processfunc::SendCommand(c.proc, "isready");
  }

  if(ponder_nums[0] == 0){
    ponder_nums[0] = 1;
    ponder_nums[1] = agents.size() -1;
  }
  cout<<"ponder_nums "<<ponder_nums[0]<<" "<<ponder_nums[1]<<endl;
}

void agent::SendCommand(const string str){
  if(status == agent_dead){
    return;
  }
  proc.addLine("(input) " + str);
  processfunc::SendCommand(proc, "%s", str.c_str());
  
}

bool agent::updateLine(){
  if(status == agent_dead){
    return false;
  }
  return proc.updateLine();
}
