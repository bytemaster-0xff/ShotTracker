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
int lowThreshold = 24;
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

std::vector<Vec4i> GetLines(byte* buffer, unsigned int size)
{
	std::vector<char> data(buffer, buffer + size);

	Mat src, gray, detectedEdges;

	Mat input = Mat(data);

	src = imdecode(input, IMREAD_COLOR);

	cvtColor(src, gray, CV_BGR2GRAY);

	dst.create(src.size(), src.type());

	blur(gray, detectedEdges, Size(3, 3));

	GaussianBlur(detectedEdges, detectedEdges, Size(3, 3), 2, 2);

	lowThreshold = 50;

	Canny(detectedEdges, detectedEdges, lowThreshold, lowThreshold*ratio, kernel_size);

	GaussianBlur(detectedEdges, detectedEdges, Size(3, 3), 2, 2);

	std::vector<Vec4i> lines;
	HoughLinesP(detectedEdges, lines, 1, CV_PI / 180, 50, 5, 5);

	return lines;

}

int totalBytesSent = 0;

void SendInt(SOCKET socket, int value)
{
	char buffer[4];
	buffer[0] = value & 0xFF;
	buffer[1] = (value >> 8) & 0xFF;
	buffer[2] = (value >> 16) & 0xFF;
	buffer[3] = (value >> 24) & 0xFF;

	int bytesSent = send(socket, buffer, 4, 0);

	totalBytesSent += bytesSent;
}

void SendVector(SOCKET socket, Vec4i vector)
{
	SendInt(socket, vector[0]);
	SendInt(socket, vector[1]);
	SendInt(socket, vector[2]);
	SendInt(socket, vector[3]);
}

void ProcessSocket(void *param)
{
	SOCKET ClientSocket = (SOCKET)param;

	printf("Accepted Connection\n");
	int result;

	int recvbuflen = RECV_BUFFER_SIZE;

	char recvbuf[RECV_BUFFER_SIZE];
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
						std::vector<Vec4i> lines = GetLines(imgBuffer, incomingImageSize);
						SendInt(ClientSocket, lines.size());
						for (size_t i = 0; i < lines.size(); i++)
						{
							Vec4i line = lines[i];
							SendVector(ClientSocket, line);
							if (i % 100 == 0)
							{
								printf("Sent %d lines\n", i);
							}
						}

						printf("Sent %d total lines and %d bytes\n", lines.size(), totalBytesSent);

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
http://www.bandgap.cs.rice.edu/classes/comp410/resources/Using%20Azure/WAPTK/Labs/WindowsAzureNativeVS2010/Lab.html/html/docSet_597aed2d-2ad5-4da2-8ecc-f0c7ba5e1eb1.html
*/
int main(int, char** argv)
{
	//_beginthread(StartListening, 1024, NULL);

	StartListening(NULL);
	/// Wait until user exit program by pressing a key
	waitKey(0);

	return 0;
}

int main1(int argc, char** argv)
{
	Mat src, gray;
	if (argc != 2)
		return -1;

	src = imread(argv[1], IMREAD_COLOR); // Load an image

	cvtColor(src, gray, CV_BGR2GRAY);

	dst.create(src.size(), src.type());

	blur(gray, detected_edges, Size(3, 3));

	GaussianBlur(detected_edges, detected_edges, Size(3, 3), 2, 2);

	lowThreshold = 50;

	Canny(detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size);

	GaussianBlur(detected_edges, detected_edges, Size(3, 3), 2, 2);

	std::vector<Vec4i> lines;
	HoughLinesP(detected_edges, lines, 1, CV_PI / 180, 50, 5, 5);
	for (size_t i = 0; i < lines.size(); i++)
	{
		Vec4i l = lines[i];
		line(dst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 0, 255), 3, CV_AA);
		line(src, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 3, CV_AA);
	}

	namedWindow("original", 1);
	imshow("original", src);

	namedWindow("edge", 1);
	imshow("edge", detected_edges);

	namedWindow("output", 1);
	imshow("output", dst);
	waitKey(0);
	return 0;
}