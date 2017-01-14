#include "Server.h"
#include <process.h>



Server::Server()
{
	m_bIsListening = false;
}

void Server::HandleRequest(void *socket) {

}


bool Server::GetIsListening() {
	return m_bIsListening;
}

void Server::StartListening(void *params)
{
	WSADATA wsaData;
	int iResult = 0;

	SOCKET listenSocket = INVALID_SOCKET;
	sockaddr_in service;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup() failed with error: %d\n", iResult);
		m_bIsListening = false;
		return;
	}
	//----------------------
	// Create a SOCKET for listening for incoming connection requests.
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		m_bIsListening = false;
		return;
	}
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	service.sin_port = htons(27015);

	iResult = bind(listenSocket, (SOCKADDR *)& service, sizeof(service));
	if (iResult == SOCKET_ERROR) {
		wprintf(L"bind function failed with error %d\n", WSAGetLastError());
		iResult = closesocket(listenSocket);
		if (iResult == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		WSACleanup();
		m_bIsListening = false;
		return;
	}

	m_bRunning = true;

	while (m_bRunning)
	{
		SOCKET acceptedSocket;
		//----------------------
		// Listen for incoming connection requests 
		// on the created socket
		if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
			wprintf(L"listen function failed with error: %d\n", WSAGetLastError());

		wprintf(L"Listening on socket...\n");

	//	acceptedSocket = accept(listenSocket, NULL, NULL);

		if (acceptedSocket != INVALID_SOCKET) {
			_beginthread(this->HandleRequest, 1024, &acceptedSocket);
		}	
	}

	iResult = closesocket(listenSocket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	WSACleanup();
}

Server::~Server()
{

}
