#pragma once
#include <map>
#include <uv.h>
#include "MMemoryPool.hpp"

class MClientSession;
class PacketPool;
class MTcpServer;

#define DEFAULT_BACKLOG 128

typedef void(*OnNewSessionCallback)(MClientSession&);
typedef void(*OnNewPacketCallback)(MClientSession&, char *, ssize_t);
typedef void(*OnSessionCloseCallback)(MClientSession&);
typedef void(*OnServerCloseCallback)(MTcpServer&);

typedef std::map<uint64_t, MClientSession*> SESSIONS_MAP;

/*
 * MTcpServer
 * use the callbacks for connection management and message handling
 * Note: This server is totally thread unsafe, when multithreading with the callbacks and send method,
 *       sessions and packets need to be queued or locked 
 */
class MTcpServer
{
public:
	MTcpServer();

	~MTcpServer();

	//set ip and port for server
	int Setup(const char * ip, int port, bool is_ip_v6 = false);

	//tcp simulataneous accepts
	int SetSimultaneousAccepts(bool enable);

	//tcp keep alive
	int SetKeepAlive(bool enable, int delay);

	//close connection after sending response
	void SetShortConnection(bool enable);

	//run server
	int Start();

	//shutdown server
	void Close();

	//shutdown a client by its id
	void CloseClient(uint64_t id);

	//send pakcet to target client
	void SendTo(uint64_t client_id, const char* data, uint64_t len);

	//send pakcet to all clients
	void Broadcast(const char * data, uint64_t len);

	//error log
	const char* GetLastError();
	static const char* GetError(int error);

	//return main loop
	uv_loop_t *GetLoop();

	//return libuv server object
	uv_tcp_t *GetServer();

	//return memory_pool
	MMemoryPool *GetMemoryPool();

	//session callbacks
	OnNewSessionCallback on_new_session;
	OnNewPacketCallback on_new_packet;
	OnSessionCloseCallback on_session_close;

	//called when ready to dispose
	OnServerCloseCallback on_server_close;
protected:
	//when new connection arrived
	void OnNewConnection(uv_stream_t * server, int status);

private:
	//log error
	static void PrintError(const char* format, int error);

	//main server loop
	uv_loop_t loop;

	//server object
	uv_tcp_t server;

	//for managing memory more efficiently
	MMemoryPool memory_pool;

	//libuv error code
	int lastError;

	//whether should be closed after response or not
	bool is_short_connection;

	//client sessions <id, session>
	SESSIONS_MAP sessions;
};