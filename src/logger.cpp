#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <ctime>    // std::ctime()

using namespace std;


// logger is copied from YaneuraOu

// --------------------
//  logger
// --------------------

// logging用のhack。streambufをこれでhookしてしまえば追加コードなしで普通に
// cinからの入力とcoutへの出力をファイルにリダイレクトできる。
// cf. http://groups.google.com/group/comp.lang.c++/msg/1d941c0f26ea0d81
struct Tie : public streambuf
{
  Tie(streambuf* buf_ , streambuf* log_) : buf(buf_) , log(log_) {}

  int sync() { return log->pubsync(), buf->pubsync(); }
  int overflow(int c) { return write(buf->sputc((char)c), "<< "); }
  int underflow() { return buf->sgetc(); }
  int uflow() { return write(buf->sbumpc(), ">> "); }

  int write(int c, const char* prefix) {
    static int last = '\n';
    if (last == '\n')
      log->sputn(prefix, 3);
    return last = log->sputc((char)c);
  }

  streambuf *buf, *log; // 標準入出力 , ログファイル
};

struct Logger {
	static void start(bool b)
	{
	  static Logger log;
	  std::time_t rawtime;
	  std::tm* timeinfo;
	  std::time(&rawtime);
	  timeinfo = std::localtime(&rawtime);
		if (b && !log.file.is_open())
		{
		  log.file.open("io_log" + to_string(timeinfo->tm_mon) + to_string(timeinfo->tm_mday) + to_string(timeinfo->tm_hour) + to_string(timeinfo->tm_min) + ".txt", ifstream::out);
			cin.rdbuf(&log.in);
			cout.rdbuf(&log.out);
			cout << "start logger" << endl;
		}
		else if (!b && log.file.is_open())
		{
			cout << "end logger" << endl;
			cout.rdbuf(log.out.buf);
			cin.rdbuf(log.in.buf);
			log.file.close();
		}
	}

private:
	Tie in, out;   // 標準入力とファイル、標準出力とファイルのひも付け
	ofstream file; // ログを書き出すファイル

	// clangだとここ警告が出るので一時的に警告を抑制する。
#pragma warning (disable : 4068) // MSVC用の不明なpragmaの抑制
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
	Logger() : in(cin.rdbuf(), file.rdbuf()), out(cout.rdbuf(), file.rdbuf()) {}
#pragma clang diagnostic pop

	~Logger() { start(false); }
};

void start_logger(bool b) { Logger::start(b); }
