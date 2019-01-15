#include <iostream>
#include <vector>
#include <mutex>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/asio.hpp>


using namespace std;
using namespace boost::asio;

io_service service;

struct talk_to_svr
{
	talk_to_svr(const std::string & username): sock_(service), started_(true), username_(username) {}
	
	void connect(ip::tcp::endpoint ep){
		sock_.connect(ep);
	}
	
	void loop(){
		write("login " + username_ + "\n");
		read_answer();
		while (started_){
			write_request();
			read_answer();
			boost::this_thread::sleep(boost::posix_time::millisec(rand() % 4000));
		}
	}
	std::string username() const { return username_; }
    
    void write_request(){
	    write("ping\n");
    }
    void read_answer(){
    	already_read_ =	read(sock_, buffer(buff_),
    	boost::bind(&talk_to_svr::read_complete,this,_1, _2));
    	process_msg();
    }
    void process_msg(){
    	std::string msg(buff_, already_read_);

    	if ( msg.find("login ") == 0) on_login(msg);
    	else if ( msg.find("ping ") == 0) on_ping(msg);
    	else if ( msg.find("clients ") == 0) on_clients(msg);
    	else std::cerr << "invalid msg " << msg << std::endl;
    }

    void on_login(const std::string& msg) { 
        std::cout<<msg<<std::endl;
        do_ask_clients();
        std::cout<<std::endl;
    }
    
    void on_ping(const std::string & msg){
        std::cout<<msg<<std::endl;
    	std::istringstream in(msg);
    	std::string answer;
    	in >> answer >> answer;
    	if ( answer == "client_list_changed")
    	    do_ask_clients();
    }
    void on_clients(const std::string & msg){
    	std::string clients = msg.substr(8);
    	std::cout << username_ << ", new client list:" << clients << std::endl;
    }
    void do_ask_clients(){
    	write("ask_clients\n");
    	read_answer();
    }
    void write(const std::string& msg) {
        sock_.write_some(buffer(msg)); 
    }
    
    size_t read_complete(const boost::system::error_code & err, size_t bytes){
    	std::cout<<bytes<<std::endl;
	    if (err) return 0;
		bool found = std::find(buff_, buff_ + bytes, '\n') < buff_ + bytes;
		return found ? 0 : 1;
    }
    
private:
	ip::tcp::socket sock_;
	enum { max_msg = 1024 };
	int already_read_;
	char buff_[max_msg];
	bool started_;
	std::string username_;
};


void run_client(const std::string & client_name){
    ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
	talk_to_svr client(client_name);
	try{
		client.connect(ep);
		client.loop();
	}
	catch(boost::system::system_error & err){
		std::cout << "client terminated " << std::endl;
	}
}

int main(){
    run_client("dima");
    //run_client("vasya");
    //run_client("lenya");



    return 0;
}
