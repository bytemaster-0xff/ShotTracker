/**
* @file CannyDetector_Demo.cpp
* @brief Sample code showing how to detect edges using the Canny Detector
* @author OpenCV team
*/

#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <process.h>

using namespace cv;

#pragma comment (lib, "Ws2_32.lib")

#define RECV_BUFFER_SIZE 2048
#define HOLDING_BUFFER_SIZE 1024
#define IMAGE_BUFFER_SIZE 1024 * 1024 * 10 /* 10MB...yeah...memory is cheap these days, allocate 10 MB to hold the uploaded image*/

#define DEFAULT_PORT "27015"

//![variables]
Mat src, src_gray;
Mat dst, detected_edges;

int edgeThresh = 1;
int lowThreshold;
int const max_lowThreshold = 100;
int ratio = 3;
int kernel_size = 3;
const char* window_name = "Edge Map";

#define SOH  0x01
#define STX  0x02
#define ETX  0x03
#define EOT  0x04



static void CannyThreshold(int, void*)
{
	//![reduce_noise]
	/// Reduce noise with a kernel 3x3
	blur(src_gray, detected_edges, Size(3, 3));
	//![reduce_noise]

	//![canny]
	/// Canny detector
	Canny(detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size);
	//![canny]

	/// Using Canny's output as a mask, we display our result
	//![fill]
	dst = Scalar::all(0);
	//![fill]

	//![copyto]
	src.copyTo(dst, detected_edges);
	//![copyto]

	//![display]
	imshow(window_name, dst);
	//![display]
}



enum ParserStates
{
	ParserStates_Idle,
	ParserStates_SohRead,
	ParserStates_StxRead,
	ParserStates_WaitingForEtx,
	ParserStates_EtxRead,
	ParserStates_WaitingForEot,
	ParserStates_EotRead
};

void ProcessImage(byte* buffer, unsigned int size)
{

	std::vector<char> data(buffer, buffer + size);

	Mat input = Mat(data);

	src = imdecode(input, IMREAD_COLOR);
	//![create_mat]
	/// Create a matrix of the same type and size as src (for dst)
	dst.create(src.size(), src.type());
	//![create_mat]

	//![convert_to_gray]
	cvtColor(src, src_gray, COLOR_BGR2GRAY);
	//![convert_to_gray]

	//![create_window]
	namedWindow(window_name, WINDOW_AUTOSIZE);
	//![create_window]

	//![create_trackbar]
	/// Create a Trackbar for user to enter threshold
	createTrackbar("Min Threshold:", window_name, &lowThreshold, max_lowThreshold, CannyThreshold);
	//![create_trackbar]

	/// Show the image
	//![reduce_noise]
	/// Reduce noise with a kernel 3x3
	blur(src_gray, detected_edges, Size(3, 3));
	//![reduce_noise]

	//![canny]
	/// Canny detector
	Canny(detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size);
	//![canny]

	/// Using Canny's output as a mask, we display our result
	//![fill]
	dst = Scalar::all(0);
	//![fill]

	//![copyto]
	src.copyTo(dst, detected_edges);
	//![copyto]

	//![display]
	imshow(window_name, dst);

	waitKey(0);


}

void ProcessSocket(void *param)
{
	SOCKET ClientSocket = (SOCKET)param;

	printf("Accepted Connection\n");
	int result;
	int sendResult;

	int recvbuflen = RECV_BUFFER_SIZE;

	char recvbuf[RECV_BUFFER_SIZE];
	char tmpBuffer[HOLDING_BUFFER_SIZE];
	byte *imgBuffer = new byte[IMAGE_BUFFER_SIZE];
	int readIndex = 0;

	unsigned int incomingImageSize = 0;
	int currentImageOffsetIndex = 0;
	unsigned short checksum = 0;
	unsigned short calcChecksum = 0;

	ParserStates parserState = ParserStates_Idle;
	do {
		printf("Starting to listen\n");
		result = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (result > 0) {
			printf("Bytes received: %d\n", result);

			for (int idx = 0; idx < result; ++idx)
			{
				unsigned char ch = (unsigned char)recvbuf[idx];
				switch (parserState)
				{
				case ParserStates_Idle:
					if (recvbuf[idx] == SOH)
					{
						readIndex = 0;
						parserState = ParserStates_SohRead;
						printf("Found SOH, started parsing \n");
					}
					break;
				case ParserStates_SohRead:

					
					switch (readIndex++)
					{
					case 0: incomingImageSize = ch | incomingImageSize; break;
					case 1: incomingImageSize = (ch << 8) | incomingImageSize; break;
					case 2: incomingImageSize = (ch << 16) | incomingImageSize; break;
					case 3:
						incomingImageSize = (ch << 24) | incomingImageSize;
						break;
					case 4:
						if (recvbuf[idx] == STX)
						{
							parserState = ParserStates_StxRead;
							currentImageOffsetIndex = 0;
							printf("Found STX, Image Size %d\n", incomingImageSize);
						}
						break;
					}

					printf("Image Size %d\n", incomingImageSize);

					break;
				case ParserStates_StxRead:
					imgBuffer[currentImageOffsetIndex++] = ch;
					calcChecksum += ch;
					if (currentImageOffsetIndex == incomingImageSize)
					{
						parserState = ParserStates_WaitingForEtx;
						printf("Image Read Completed\n");
					}
					break;
				case ParserStates_WaitingForEtx:
					if (ch == ETX)
					{
						parserState = ParserStates_WaitingForEot;
						readIndex = 0;
						printf("Read ETX, waiting for check sum\n");
					}
					break;
				case ParserStates_WaitingForEot:
					printf("READING BYTE %d %d\n", readIndex, ch);
					switch (readIndex++) {
					case 0: checksum = ch;  break;
					case 1: checksum = (ch << 8) | checksum;  break;
					case 2:if (recvbuf[idx] == EOT)
						printf("Read Check sum %d - %d and EOT - All Done\n", checksum, calcChecksum);
						send(ClientSocket, "OK", 2, 0);
						break;
					}
					break;
				}
			}
		}
		else if (result == 0) {
			printf("Connection closing...\n");
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return;
		}

	} while (result > 0);



	// shutdown the connection since we're done
	result = shutdown(ClientSocket, SD_SEND);
	if (result == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return;
	}

	ProcessImage(imgBuffer, incomingImageSize);
	// cleanup
	closesocket(ClientSocket);

	printf("Socket shut down successful\r\n");
}

void StartListening(void *)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	printf("Started Listening on %s\n", DEFAULT_PORT);

	while (true) {
		printf("Waiting For Accept\n");
		SOCKET ClientSocket = INVALID_SOCKET;
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return;
		}
		else
		{
			_beginthread(ProcessSocket, 1024, (void*)ClientSocket);
		}
	}



	// No longer need server socket
	closesocket(ListenSocket);


	WSACleanup();
}


/**
* @function main
*/
int main(int, char** argv)
{
	//_beginthread(StartListening, 1024, NULL);

	StartListening(NULL);
	/// Wait until user exit program by pressing a key
	waitKey(0);

	return 0;
}