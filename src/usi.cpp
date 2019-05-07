#include "agent.h"
#include "misc.h"

using namespace std;

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <string>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include "unistd.h"
#include "pvparse.h"
#include "logger.h"
#include <map>


// todo 致命的にリファクタが必要

void flush_agent(agent &a){
  cout<<"flush agent "<<a.agentname<<" "<<a.posstr<<endl;
  if(a.status == agent_idle){
    cout<<"agent idle"<<endl;
    return;
  }
  if (a.status == agent_dead){
    cout<<"agent_dead"<<endl;
    return;
  }
  const int timeout = 100000;
  // send stop command and discard outputs
  clock_t start_time = clock();
  a.SendCommand("stop");
  while(1){
    bool isupdate = a.updateLine();
    if (isupdate){
      string mess = a.proc.getLine();
    
      if(mess.find("bestmove") != string::npos){
	a.status = agent_idle;
	break;
      }
    }
    if(int(clock()-start_time) > timeout){
      cout<<"agent timeout "<<endl;
      // bestmoveを返さなかったslaveはbestmoveを返すまで休止させる
      a.status = agent_dead;
      a.posstr = "undefined";
      break;
    }
    usleep(10);
  }
}

bool isExist(const std::string& str){
  std::ifstream ifs(str);
  return ifs.is_open();
}

bool pos_is_black(const string posstr){
  // judge pos is black or white by just counting number of moves
  istringstream is(posstr);
  string token;
  is >> token;
  bool output = true;
  bool count = false;
  if ( token=="position" ){
    // position startpos moves...
    while(1){
      token = "";
      is >> token;
      if (token==""){
	break;
      }else if(token=="moves"){
	count = true;
      }else {
	if (count){
	  output = !output;
	}
      }
    }
    if (!count){
      cout<<"pos_is_black : error invalid posstr "<<posstr<<endl;
      return true;
    }else{
      return output;
    }
  }else{
    // sfen
    cout<<"pos_is_black : todo write code for sfen "<<posstr<<endl;
    return true;
  }
  cout<<"pos_is_black : unreachable "<<posstr<<endl;
  return true;
}

string parse_ponder_cmd(agentmaster &master, const string &prev_go_cmd, const bool isblack, const int time_minus, const bool preponder_fisher){
  string token;
  istringstream is(prev_go_cmd);
  
  int btime = 0;
  int wtime = 0;
  int winc = 0;
  int binc = 0;
  int byoyomi = 0;
  int mytime = 0;
  while (is >> token){
    if (token == "btime")     is >> btime;
    else if (token == "wtime")     is >> wtime;
    // フィッシャールール時における時間
    else if (token == "winc")      is >> winc;
    else if (token == "binc")      is >> binc;
    // 秒読み設定。
    else if (token == "byoyomi") {
      is >> byoyomi;  
    }
    
  }
  if (isblack){
    btime -= time_minus;
    if (btime < 0){
      cout << "alert : btime is minus"<<btime<<","<<time_minus<<endl;
      btime = 0;
    }
    mytime = btime;
  }else{
    wtime -= time_minus;
    if (wtime < 0){
      cout << "alert : wtime is minus"<<wtime<<","<<time_minus<<endl;
      wtime = 0;
    }
    mytime = wtime;
  }

  // cirital ponderhitならoptimumを減らす（白ビール戦略）
  if (preponder_fisher){
    /*
      btime = btime / 2;
      wtime = wtime / 2;
    */
  }

  // 残り１分未満ならponderhitは即差し
  if(btime < 60000 || wtime < 60000){
    btime = 10;
    wtime = 10;
    // 終盤にdepth6とかdepth8で探索するとdistのコストで時間切れになるリスクがある
    cout<<"depth change to shortage"<<endl;
    master.diststr = "go depth 6";
  }

  return "go ponder btime " + to_string(btime) + " wtime " + to_string(wtime) + " binc "+to_string(binc) + " winc "+to_string(winc) + " byoyomi "+to_string(byoyomi);

}


vector<packed_pv> generate_dfs(agentmaster &master, string cmd, string pvstr, int depth, int maxdepth){
  const string go_generator = master.diststr;
  packed_pv pv;
  vector<packed_pv> pvvec;
  vector<packed_pv> pvvec_out;
  if (pvstr == "resign " || pvstr == "win "){
    return pvvec_out;
  }
  master.agents[master.gid].SendCommand(cmd+" "+pvstr);
  master.agents[master.gid].SendCommand(go_generator);
  while(true){
    bool isupdate = master.agents[master.gid].updateLine();
    if (isupdate){
      string mess = master.agents[master.gid].proc.getLine();
      replace(mess, "\n","");
      if(mess.find("bestmove") != string::npos){
	break;
      }else{
	pv = pvparse::pvparse(mess);
	if(pv.multipv+1 > pvvec.size()){
	  pvvec.resize(pv.multipv+1);
	}
	pvvec[pv.multipv] = pv;
      }
    }
    usleep(10);
  }
  int dfs_cnt = 0;
  for(auto pvs : pvvec){
    // debug
    if(depth<maxdepth){
      // append pvvec_out
      if (pvs.pv == "resign" || pvs.pv == "win"){
	continue;
      }

      if(dfs_cnt < master.ponder_nums[depth]){
	vector<packed_pv> pv_child = generate_dfs(master, cmd, pvstr+ pvs.pv+" ", depth+1, maxdepth);
	std::copy(pv_child.begin(),pv_child.end(),std::back_inserter(pvvec_out));
	dfs_cnt += 1;
      }
    }
  }
  if (depth<maxdepth){
    return pvvec_out;
  }
  
  for(auto &pvc : pvvec){
    pvc.pv = pvstr + pvc.pv;
  }
  if(maxdepth > 0){
    pvvec.resize(min(int(pvvec.size()), master.ponder_nums[1]));
  }
  return pvvec;
}

void go_cmd(agentmaster &master, string cmd){
  cout<<"go cmd : "<<cmd<<endl;
  {
    master.prev_go_cmd = cmd;
    master.start_time = clock();
  }
  master.rid = -1;
  {
    int itr = 0;
    for(auto &a : master.agents){
      if(a.posstr == master.posstr){
	// ponder hit
	// todo check usi1 rule for ponder
	cout<<"ponder hit found : "<<itr<<endl;
	master.agents[itr].SendCommand("ponderhit");
	master.rid = itr;
      }
      ++itr;
    }
  }
  if (master.rid == -1){
    cout<<"no ponder hit found"<<endl;
    {
      int itr = 0;
      for(auto &a : master.agents){

	// flush log
	flush_agent(a);
	if(itr != master.gid && a.status != agent_dead){
	  a.SendCommand(master.posstr);
	  a.SendCommand(cmd);
	  cout<<"push " << itr <<" "<<master.posstr<<endl;
	  cout<<"push " << itr <<" "<<cmd<<endl;
	  a.status = agent_go;
	  master.rid = itr;
	  break;
	}
	++itr;
      }
    }
  }
  
  // distrubution
  vector<packed_pv> pvs = generate_dfs(master, master.posstr, "", 0, 1);
  for(auto pv : pvs){
    cout<<"generated pvs "<<pv<<endl;
  }

  bool pvs_used[pvs.size()];
  bool agent_used[master.agents.size()];
  for(int j=0;j<pvs.size();++j){
    pvs_used[j]=false;
  }
  for(int i=0;i<master.agents.size();++i){
    agent_used[i]=false;
    if(i==master.gid || i==master.rid){
      continue;
    }
    for(int j=0;j<pvs.size();++j){
      if(master.posstr + " "+ pvs[j].pv == master.agents[i].posstr){
	cout<<"gocmd distribute : hit "<<master.agents[i].posstr<<endl;
	pvs_used[j]=true;
	agent_used[i]=true;
      }
    }
  }
  for(int i=0;i<master.agents.size();++i){
    if(i==master.gid || i==master.rid || agent_used[i]){
      continue;
    }
    flush_agent(master.agents[i]);
    for(int j=0;j<pvs.size();++j){
      if(pvs_used[j]){
	continue;
      }
      if ((master.posstr + " " + pvs[j].pv).find("win")  != string::npos || (master.posstr + " " + pvs[j].pv).find("resign")  != string::npos){
	continue;
      }
      master.agents[i].SendCommand(master.posstr + " " + pvs[j].pv);
      cout<<"distribute : push "<<i<<" "<<master.posstr + " " + pvs[j].pv<<endl;
      string ponder_cmd = parse_ponder_cmd(master, master.prev_go_cmd, pos_is_black(master.posstr + " "+ pvs[j].pv), 0, true);
      cout<<"distribute : cmd "<<ponder_cmd<<endl;
      master.agents[i].SendCommand(ponder_cmd);
      master.agents[i].status = agent_ponder;

      master.agents[i].posstr = master.posstr + " "+ pvs[j].pv;
      agent_used[i]=true;
      pvs_used[j]=true;
      break;
    }
  }
}

void bestmove_cmd(agentmaster &master, string cmd){
  // redistribute pos
  istringstream is(cmd);
  cout<<"raw bm cmd : "<<cmd<<endl;
  string bmstr;
  string ponderstr="";
  is >> bmstr; is >> bmstr;
  // output bestmove without ponder
  // never send ponder to avoid terrible usi protocol
  cout<<"bestmove "<<bmstr<<endl;
  is >> ponderstr;  is >> ponderstr;
  // distrubution
  vector<packed_pv> pvs = generate_dfs(master, master.posstr, bmstr+" ", 0, 0);
  // special action for ponder move
  if (ponderstr != ""){
    packed_pv next_pv;
    packed_pv buff_pv;
    next_pv.pv = bmstr+ " " + ponderstr;
    for(int i=0;i<pvs.size();++i){
      buff_pv = pvs[i];
      pvs[i] = next_pv;
      next_pv = buff_pv;
      if (buff_pv.pv == bmstr+ " "+ ponderstr){
	break;
      }
    }
  }
  for(auto pv : pvs){
    cout<<"generated pvs "<<pv<<endl;
  }
  master.rid = -1;
  bool pvs_used[pvs.size()];
  bool agent_used[master.agents.size()];
  for(int j=0;j<pvs.size();++j){
    pvs_used[j]=false;
  }
  for(int i=0;i<master.agents.size();++i){
    agent_used[i]=false;
    if(i==master.gid || master.agents[i].status == agent_dead){
      continue;
    }
    for(int j=0;j<pvs.size();++j){
      if(master.posstr + " "+ pvs[j].pv == master.agents[i].posstr){
	cout<<"bmcmd distribute : hit " <<i<<" "<<master.agents[i].posstr<<endl;
	pvs_used[j]=true;
	agent_used[i]=true;
      }
    }
  }
  long long int time_minus = (1000 * (clock() - master.start_time) / CLOCKS_PER_SEC); // microsecond ?
  cout<<"distribute: used time = "<<time_minus<<endl;
  for(int i=0;i<master.agents.size();++i){
    if(i==master.gid || agent_used[i] || master.agents[i].status == agent_dead){
      continue;
    }
    flush_agent(master.agents[i]);
    for(int j=0;j<pvs.size();++j){
      if(pvs_used[j]){
	continue;
      }
      if ((master.posstr + " " + pvs[j].pv).find("win")  != string::npos || (master.posstr + " " + pvs[j].pv).find("resign")  != string::npos){
	continue;
      }
      master.agents[i].SendCommand(master.posstr + " " + pvs[j].pv);
      cout<<"distribute : push "<<i<<" "<<master.posstr + " " + pvs[j].pv<<endl;
      cout<<"distribute : ponder_cmd(before) "<<i<<" "<<master.prev_go_cmd<<endl;
      string ponder_cmd = parse_ponder_cmd(master, master.prev_go_cmd, pos_is_black(master.posstr + " "+ pvs[j].pv), int(time_minus), false);
      cout<<"distribute : ponder_cmd "<<ponder_cmd<<endl;
      master.agents[i].SendCommand(ponder_cmd);
      master.agents[i].status = agent_ponder;
      master.agents[i].posstr = master.posstr + " "+ pvs[j].pv;
      agent_used[i]=true;
      pvs_used[j]=true;
      break;
    }
  }
}

void usi::loop(agentmaster &master){

  // hacks for non blocking
  struct termios save_settings;
  struct termios settings;

  tcgetattr(0,&save_settings);
  settings = save_settings;

  //settings.c_lflag &= ~(ECHO|ICANON);  /* 入力をエコーバックしない、バッファリングしない */
  settings.c_cc[VTIME] = 0;
  settings.c_cc[VMIN] = 1;
  tcsetattr(0,TCSANOW,&settings);
  fcntl(0,F_SETFL,O_NONBLOCK);/* 標準入力から読み込むときブロックしないようにする */
  
  string cmd;
  string tout = "";
  bool boot_done = false;
  int boot_count = 0;
  bool usi_to_answer = false;
  bool isready_to_answer = false;
  do{
    cmd = getstr();
    if(cmd != ""){
      //cout<<cmd<<endl;
    }
    istringstream is(cmd);
    string token;
    bool iswaitres = false;
    bool ispart = true;
    string key;
    
    is >> token;
    
    if(token == "quit"){
      for(auto &c: master.agents){
	c.status = agent_idle;
	c.SendCommand("quit");
      }
      break;
    }else if(token=="usi"){
      if (boot_done){
	cout<<"id name "<<master.idstr<<endl;
	cout<<"id author "<<master.author<<endl;
	cout<<"usiok"<<endl;
      }else{
	usi_to_answer = true;
      }
    }else if(token=="usinewgame"){
      
    }else if(token=="isready"){
      if(boot_done){
	const bool reboot = false;
	if (reboot){
	  
	  cout<<"agent reboot"<<endl;
	  // reboot
	  boot_done = false;
	  isready_to_answer = true;
	  boot_count = 0;
	  for(auto &c: master.agents){
	    c.status = agent_idle;
	    c.SendCommand("quit");
	  }
	  master.gid = -1;
	  master.rid = -1;
	  master.agents.clear();
	  string fileconfig = "config.txt";
	  ifstream ifs(fileconfig);
	  master.init(ifs);
	  ifs.close();
	  
	}else{
	  for(auto &c: master.agents){
	    flush_agent(c);
	  }
	  cout<<"readyok"<<endl;
	}
       

      }else{
	isready_to_answer = true;
      }
    }else if(token == "position"){
      // positionにmovesがないと色々死ぬのでmovesがない場合はmovesを足す。
      // sfenの検討...あきらめろん
      int mvpos = cmd.find("moves");
      if (mvpos == std::string::npos) {
	master.posstr = cmd + " moves";
      }else{
	master.posstr = cmd;
      }
    }else if(token == "stop"){
      // stop if needed
    }else if(token =="go"){
      // go cmd
      if (cmd.find("ponder") == string::npos){
	go_cmd(master, cmd);
      }
    }else if(token != ""){
      // ignore
    }

    
    // loop for receive message from agent
    int itr = 0;
    for(auto &a : master.agents){
      bool isupdate = a.updateLine();
      if (isupdate){
	string mess = a.proc.getLine();
	replace(mess, "\n","");

	// receive readyok
	if(a.status == agent_boot && mess == "readyok"){
	  a.status = agent_idle;
	  a.proc.clearLines();
	  boot_count += 1;
	  cout<<a<<endl;
	  if (boot_count == master.agents.size()){
	    boot_done = true;
	    if (usi_to_answer){
	      usi_to_answer = false;
	      cout<<"id name QDM"<<endl;
	      cout<<"id author Passione Shogi Team"<<endl;
	      cout<<"usiok"<<endl;
	      usi_to_answer=false;
	    }
	    if (isready_to_answer){
	      cout<<"readyok"<<endl;
	    }
	    //cout<<"usiok"<<endl;
	  }
	}else if(mess.find("bestmove") != string::npos){
	  if(master.rid != itr){
	    /* 
	       通信の遅延などでstopを送っても即座にbestmoveが帰ってこないことがある
	       bestmoveを返さないノードはdead状態とする
	       dead状態のノードからbestmoveが帰ってきたらdeadノードをidleに戻す
	     */
	    cout<<"warning : bm from non root (dead slave reborned?)" << master.rid<<"(master) "<<itr<<"(itr)"<<mess<<"(mess) "<<endl;
	    a.status = agent_idle;
	    a.proc.clearLines();
	    continue;
	    
	  }
	  a.status = agent_idle;
	  bestmove_cmd(master, mess);
	  continue;
	}
	if(itr != master.rid){
	  // log from other node
	  cout<<itr<<"(slave) : "<<mess<<endl;
	}else{
	  cout<<mess<<endl;
	}

      }
      ++itr;
    }
    
    usleep(10);
  }while(cmd != "quit");
}
