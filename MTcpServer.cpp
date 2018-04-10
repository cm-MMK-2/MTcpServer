#include "MTcpServer.hpp"
#include "MClientSession.hpp"

MTcpServer::MTcpServer() :on_new_session(nullptr), on_new_packet(nullptr),
	on_session_close(nullptr), on_server_close(nullptr), is_short_connection(false){}

MTcpServer::~MTcpServer(){}

int MTcpServer::Setup(const char* ip, int port, bool is_ip_v6) {
	lastError = uv_loop_init(&loop);
	if (lastError) {
		PrintError("Loop initialization error : %s.\n", lastError);
		return lastError;
	}
	loop.data = &memory_pool;
	lastError = uv_tcp_init(&loop, &server);
	if (lastError) {
		PrintError("Server initialization error : %s.\n", lastError);
		return lastError;
	}
	if (!is_ip_v6)
	{
		struct sockaddr_in addr;
		lastError = uv_ip4_addr(ip, port, &addr);
		if (lastError) {
			PrintError("Server addr error : %s.\n", lastError);
			return lastError;
		}
		//bind server
		lastError = uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
		if (lastError) {
			PrintError("Server binding error : %s.\n", lastError);
			return lastError;
		}
	}
	else
	{
		struct sockaddr_in6 addr;
		lastError = uv_ip6_addr(ip, port, &addr);
		if (lastError) {
			PrintError("Server addr error : %s.\n", lastError);
			return lastError;
		}
		//bind server
		lastError = uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
		if (lastError) {
			PrintError("Server binding error : %s.\n", lastError);
			return lastError;
		}
	}
	//pass this pointer for callback
	server.data = this;
	//listen for new connection
	lastError = uv_listen((uv_stream_t *)&server, DEFAULT_BACKLOG, [](uv_stream_t * server, int status) {
		((MTcpServer *)server->data)->OnNewConnection(server, status);
	});
	if (lastError) {
		return lastError;
	}
	return 0;
}

int MTcpServer::SetSimultaneousAccepts(bool enable)
{
	lastError = uv_tcp_simultaneous_accepts(&server, enable ? 1 : 0);
	return lastError;
}

int MTcpServer::SetKeepAlive(bool enable, int delay)
{
	lastError = uv_tcp_keepalive(&server, enable ? 1 : 0, delay);
	return lastError;
}

void MTcpServer::SetShortConnection(bool enable)
{
	is_short_connection = enable;
}

int MTcpServer::Start()
{
	if (lastError = uv_run(&loop, UV_RUN_DEFAULT)) {
		PrintError("Run loop error : %s.\n", lastError);
		return lastError;
	}
	return lastError;
}

void MTcpServer::Close()
{
	for (auto & session : sessions)
	{
		CloseClient(session.first);
	}
	uv_close((uv_handle_t *)&server, [](uv_handle_t* handle) {
		int error = uv_loop_close(handle->loop);
		if (error)
		{
			PrintError("Close server loop error : %s.\n", error);
		}
		MTcpServer* _tcpServer = (MTcpServer *)(handle->data);
		if (_tcpServer->on_server_close)
		{
			_tcpServer->on_server_close(*_tcpServer);
		}
	});
}

void MTcpServer::CloseClient(uint64_t id)
{
	MClientSession* _session = sessions[id];
	if (on_session_close)
	{
		on_session_close(*_session);
	}
	uv_close((uv_handle_t *)_session->GetClient(), [](uv_handle_t* handle) {
		delete (MClientSession*)handle->data;
	});
	//only erase from map, not clear memory
	sessions.erase(id);
}

void MTcpServer::SendTo(uint64_t client_id, const char * data, uint64_t len)
{
	sessions[client_id]->Send(data, len);
}

void MTcpServer::Broadcast(const char * data, uint64_t len)
{
	//loop map
	for (auto &session: sessions)
	{
		session.second->Send(data, len);
	}
}

const char* MTcpServer::GetLastError()
{
	return uv_strerror(lastError);
}

const char * MTcpServer::GetError(int error)
{
	return uv_strerror(error);
}

void MTcpServer::PrintError(const char* format, int error)
{
	fprintf(stderr, format,
		uv_strerror(error));
}

uv_loop_t * MTcpServer::GetLoop()
{
	return &loop;
}

uv_tcp_t * MTcpServer::GetServer()
{
	return &server;
}

MMemoryPool * MTcpServer::GetMemoryPool()
{
	return &memory_pool;
}

void MTcpServer::OnNewConnection(uv_stream_t * server, int status) {
	if (status < 0) {
		lastError = status;
		PrintError("Connection status error: %s.\n", lastError);
		return;
	}
	//get a unique id for session
	uint64_t session_id = MClientSession::CreateID();
	MClientSession *_session = (sessions[session_id] = new MClientSession(session_id, this));
	_session->SetShortConnection(is_short_connection);
	lastError = _session->StartAccept();
	if (lastError)//accept error, close connection, do not call session close callback
	{
		PrintError("Accept connection error: %s.\n", lastError);
		uv_close((uv_handle_t *)_session->GetClient(), [](uv_handle_t* handle) {
			delete (MClientSession*)handle->data;
		});
		//only erase from map, not clear memory
		sessions.erase(session_id);
	}
}

