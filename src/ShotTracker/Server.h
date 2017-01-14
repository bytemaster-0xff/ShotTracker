#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT 27000

class Server
{
private:
	bool m_bIsListening;
	bool m_bRunning;

public:
	Server();
	~Server();

	void StartListening(void *params);
	void HandleRequest(void* socket);
	bool GetIsListening();
};

