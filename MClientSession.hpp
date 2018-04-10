#pragma once
#include <deque>
#include <uv.h>

//max buffer size should match the normal max packet size to make the server work more efficiently
#define DEFAULT_MAX_BUFFER_SIZE 8192

typedef std::deque<uv_buf_t> PACKET_QUEUE;

//forward declaration
class MTcpServer;

/*
 * MClientSession
 * long term session, life time is the same as connection
 * Use send method(thread unsafe) for sending messages to client
 */
class MClientSession
{
public:
	MClientSession(uint64_t _id, MTcpServer * _server);

	~MClientSession();

	//generate a unique id for client
	static uint64_t CreateID();

	//getter for this instance's id
	uint64_t GetID();

	uv_tcp_t* GetClient();

	//start accepting sockets
	int StartAccept();

	//shut down client, todo:add reason
	void Close();

	//send new packet from server to client
	void Send(const char *data, size_t len);

	//static size_t max_buffer_size;

	//getter for pending_packets
	PACKET_QUEUE GetPackets();

	//get ip, port information
	void GetEndPoint(char *hostname, int hostname_len, int *port);

	//whether close connection after response(according to server)
	void SetShortConnection(bool is_short);

protected:
	// called when new packet arrived
	void OnNewPacket(ssize_t nread, const uv_buf_t* buf);

	//send packets in the queue
	void SendQueuedPackets();

	//after send callback
	void OnSendFinish();

private:
	//unique id
	uint64_t id;

	//client object
	uv_tcp_t client;

	//server object
	MTcpServer *tcpServer;

	//request for sending message
	uv_write_t send_req;

	//pakcets waiting for sending
	PACKET_QUEUE pending_packets;

	//whether should be closed after response or not
	bool is_short_connection;

	//state marker for sending pakcet or not
	bool is_sending_packet;

	//id incrementor
	static uint64_t id_counter;
};