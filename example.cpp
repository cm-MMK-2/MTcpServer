#include "MClientSession.hpp"
#include "MTcpServer.hpp"
#include <string>
#include <thread>
#include <iostream>

std::string html_response("HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\n\n<HTML>test!test!test!test!test!test!test!test!test!</HTML>");

int main()
{
	using namespace std;
	thread t_long([] { 
		MTcpServer server;
		server.Setup("0.0.0.0", 9000);
		server.SetSimultaneousAccepts(true);
		server.on_new_session = [](MClientSession& session) {
			//char host[NI_MAXHOST];
			//int port;
			//session.GetEndPoint(host, NI_MAXHOST, &port);
			//cout << "new session connected, id:" << session.GetID() << ", address:" << host << ", port:" << port << endl;
		};
		server.on_new_packet = [](MClientSession& session, char * data, ssize_t len) {
			//cout << "new packet: " << string(data, len) << endl;
			session.Send(data, len);
			//session.Send(html_response.c_str(), html_response.size());
		};
		server.on_session_close = [](MClientSession& session) {
			//cout << "connection close: " << session.GetID() << endl;
		};
		server.Start();
	});

	thread t_short([] {
		MTcpServer server;
		server.Setup("0.0.0.0", 9001);
		server.SetSimultaneousAccepts(true);
		//server.SetKeepAlive(true, 60);
		server.SetShortConnection(true);
		server.on_new_packet = [](MClientSession& session, char * data, ssize_t len) {
			cout << "new packet: " << string(data, len) << endl;
			session.Send(html_response.c_str(), html_response.size());
		};
		server.Start();
	});
	
	t_long.join();
	t_short.join();
}
