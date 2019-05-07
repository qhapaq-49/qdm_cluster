#include "process.h"
#include "misc.h"

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>

std::mutex mutex_;

void processfunc::boot(Process &proc, const bool isssh, const string sshname, const string sshdir, const string binname){

  string token;
  if(isssh){
    proc.procName = "ssh " + sshname + "; cd " +sshdir+"; "+binname;
     token = "cd " + sshdir + "; "+binname;
  }else{
    proc.procName = binname;
  }
  proc.isTimeout = false;
  char* const args[] = {
    // リモートマシンをワーカーとして使用する場合: SSHで通信する
    const_cast<char*>("ssh"),
    const_cast<char*>(sshname.c_str()), // ~/.ssh/configでワーカーのホスト名を設定しておく
    const_cast<char*>(token.c_str()), NULL // ディレクトリを移動後、実行する
  };
  
  char* const argslocal[] = {
    const_cast<char*>(binname.c_str()),NULL
  };

  cout<<"boot command "<<proc.procName<<endl;
  if(isssh){
    if (proc.StartProcess(args[0], args) < 0) {
      std::cout<<"boot failed"<<std::endl;
    }
  }else{    
    if (proc.StartProcess(argslocal[0], argslocal) < 0) {
      std::cout<<"boot failed"<<std::endl;
    }
  }
  proc.init_nonblock();
  string callusi = "usi";
  SendCommand(proc, callusi.c_str());
  
}

void Process::init_nonblock(){
  int fd = fileno(stream_from_child_);
  fcntl(fd, F_SETFL, O_NONBLOCK); 
}

void Process::addLine(const string str){
  lines.push_back(str);
}

bool Process::updateLine() {
  char c;
  c = std::fgetc(stream_from_child_);
  if(c!= EOF){
    sandline = sandline+c;
  }
  if(sandline.find("\n") != string::npos){
    replace(sandline, "\n", "");
    lines.push_back(sandline);
    sandline = "";
    return true;
  }
  return false;
}

int Process::StartProcess(const char* const file, char* const argv[]) {
  const int kRead = 0, kWrite = 1;

  int pipe_from_child[2];
  int pipe_to_child[2];

  // パイプの作成（親プロセス->子プロセス）
  if (pipe(pipe_from_child) < 0) {
    std::perror("failed to create pipe_from_chlid.\n");
    return -1;
  }

  // パイプの作成（子プロセス->親プロセス）
  if (pipe(pipe_to_child) < 0) {
    std::perror("failed to create pipe_to_child.\n");
    close(pipe_from_child[kRead]);
    close(pipe_from_child[kWrite]);
    return -1;
  }

  // 子プロセスの作成
  pid_t process_id = fork();
  if (process_id < 0) {
    std::perror("fork() failed.\n");
    close(pipe_from_child[kRead]);
    close(pipe_from_child[kWrite]);
    close(pipe_to_child[kRead]);
    close(pipe_to_child[kWrite]);
    return -1;
  }

  // 子プロセスの場合、process_id はゼロとなる
  if (process_id == 0) {
    // 子プロセス側で使わないパイプを閉じる
    close(pipe_to_child[kWrite]);
    close(pipe_from_child[kRead]);

    // 親->子への出力を、標準入力に割り当てる
    dup2(pipe_to_child[kRead], STDIN_FILENO);

    // 子->親への入力を、標準出力に割り当てる
    dup2(pipe_from_child[kWrite], STDOUT_FILENO);

    // すでに標準入出力に割り当てたファイルディスクリプタを閉じる
    close(pipe_to_child[kRead]);
    close(pipe_from_child[kWrite]);

    // 子プロセスにおいて、子プログラムを起動する
    if (execvp(file, argv) < 0) {
      // プロセス起動時にエラーが発生した場合
      std::perror("execvp() failed!\n");
      return -1;
    }
  }

  // プロセスIDを記憶させる
  process_id_ = process_id;

  // パイプをファイルストリームとして開く
  stream_to_child_ = fdopen(pipe_to_child[kWrite], "w");
  stream_from_child_ = fdopen(pipe_from_child[kRead], "r");

  // バッファリングをオフにする
  // 注意：バッファリングをオフにしないと、外部プロセスとの通信が即時に行われない場合がある
  std::setvbuf(stream_to_child_, NULL, _IONBF, 0);
  std::setvbuf(stream_from_child_, NULL, _IONBF, 0);

  return process_id;
}

string Process::getLine(){
  if(lines.size()>0){
    return lines[lines.size()-1];
  }
  return "";
}

void Process::clearLines(){
  lines.clear();
}

vector<string> Process::getLines(){
  return lines;
}
