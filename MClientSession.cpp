#include <uv.h>
#include "MClientSession.hpp"
#include "MTcpServer.hpp"
#include <cstring>

uint64_t MClientSession::id_counter = 0;

MClientSession::MClientSession(uint64_t _id, MTcpServer* _server):is_sending_packet(false), id(_id), tcpServer(_server), is_short_connection(false)
{
	uv_tcp_init(tcpServer->GetLoop(), &client);
	client.data = this;
	send_req.data = this;
}

MClientSession::~MClientSession() {
	for (auto & packet : pending_packets)
	{
		((MMemoryPool *)client.loop->data)->Release(packet.base);
	}
}

uint64_t MClientSession::CreateID()
{
	//thread safe is not required
	//if the server receives 100,000,000 new connections per second,
	//it requies 18,446,744,073,709,551,615/100,000,000/60/60/24/365.25=5845.42 years to reach UINT64_MAX.
	return ++id_counter;
}

uint64_t MClientSession::GetID()
{
	return id;
}

uv_tcp_t * MClientSession::GetClient()
{
	return &client;
}

void MClientSession::SetShortConnection(bool is_short)
{
	is_short_connection = is_short;
}

int MClientSession::StartAccept()
{
	int error = uv_accept((uv_stream_t *)tcpServer->GetServer(), (uv_stream_t *)&client);  
	if (!error) {
		if (tcpServer->on_new_session)
		{
			tcpServer->on_new_session(*this);
		}
		error = uv_read_start((uv_stream_t *)&client, [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
			//allocate buffer
			buf->base = ((MMemoryPool *)handle->loop->data)->Allocate();
			buf->len = DEFAULT_MAX_BUFFER_SIZE;
		}, [](uv_stream_t * client, ssize_t nread, const uv_buf_t* buf) {
			//new packet callback
			((MClientSession *)client->data)->OnNewPacket(nread, buf);
			((MMemoryPool *)client->loop->data)->Release(buf->base);
		});
		if (error)
		{
			fprintf(stderr, "Start reading client stream error: %s.\n",
				uv_strerror(error));
			Close();
			return 0;
		}
	}
	return error;
}

void MClientSession::OnNewPacket(ssize_t nread, const uv_buf_t* buf) {
	if (nread > 0)
	{
		if (tcpServer->on_new_packet)
		{
			tcpServer->on_new_packet(*this, buf->base, nread);
		}
	}
	else if (nread < 0) {
		if (nread != UV_EOF)
		{
			fprintf(stderr, "Read client stream error: %s.\n",
				uv_strerror(nread));
		}
		Close();
	}
}

void MClientSession::Send(const char * data, size_t len)
{
	size_t offset = 0;

	//if buffer's max size is not enough, make more send event
	while (len - offset > DEFAULT_MAX_BUFFER_SIZE)
	{
		char *buf = tcpServer->GetMemoryPool()->Allocate();
		std::memcpy(buf, data + offset, DEFAULT_MAX_BUFFER_SIZE);
		pending_packets.push_back(uv_buf_init(buf, DEFAULT_MAX_BUFFER_SIZE));
		offset += DEFAULT_MAX_BUFFER_SIZE;
	}
	//last pakcet
	char * buf = tcpServer->GetMemoryPool()->Allocate();
	std::memcpy(buf, data + offset, len - offset);
	pending_packets.push_back(uv_buf_init(buf, len - offset));

	if (!is_sending_packet)
	{
		is_sending_packet = true;
		SendQueuedPackets();
	}
}

void MClientSession::SendQueuedPackets()
{
	int error = uv_write(&send_req, (uv_stream_t *)&client, &pending_packets.front(), 1, [](uv_write_t * req, int status)
	{
		MClientSession * _session = (MClientSession *)req->data;
		if (status == 0)
		{
			_session->OnSendFinish();
		}
		else
		{
			fprintf(stderr, "Write client stream callback error: %s.\n",
				uv_strerror(status));
			//_session->Close();
		}
	});
	if (error != 0)
	{
		fprintf(stderr, "Write client stream error: %s.\n",
			uv_strerror(error));
		//Close();
	}
}


void MClientSession::OnSendFinish()
{
	//pop the finished pakcet
	tcpServer->GetMemoryPool()->Release(pending_packets.front().base);
	pending_packets.pop_front();

	if (pending_packets.size())
	{
		//send more
		SendQueuedPackets();
	}
	else
	{
		is_sending_packet = false;
		//short connection should be closed after sending response
		if (is_short_connection)
		{
			Close();
		}
	}
}


void MClientSession::Close()
{
	tcpServer->CloseClient(id);
	//printf("connection: %d close\n", id);
}

void MClientSession::GetEndPoint(char* host, int host_len, int *port)
{
	sockaddr_storage addr;
	int len = sizeof(sockaddr_storage);
	uv_tcp_getpeername(&client, (sockaddr*)&addr, &len);
	char servinfo[NI_MAXSERV];
	getnameinfo((sockaddr*)&addr, sizeof(addr), host,
		host_len, servinfo, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);
	*port = atoi(servinfo);
}

PACKET_QUEUE MClientSession::GetPackets()
{
	return pending_packets;
}
