#pragma once

#include "stdafx.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

#define WS_START_ERROR 1
#define RESOLVE_HOST_ERROR 2
#define CREATE_SOCKET_ERROR 3
#define CONNECT_HOST_ERROR 4
#define RECIEVE_SOCKET_ERROR 5
#define DISCONECTED 6

#define BUFFER_SIZE 100

class ftp_client
{
private:
	WSADATA SData;
	SOCKET s;
	addrinfoW *host = NULL;

	int u_line = 0; //number of unprocessed line
	std::wstringstream response;

	char *buffer;

	bool readable(int wait_time);
	int recieve_info();

	std::wstring getline();
	SOCKET open_port();
public:
	~ftp_client();

	ftp_client(std::wstring server, int port = 21);

	void print_response(std::wostream &os);

	void dir();
	void send_command(std::wstring command);
};

//void recieve_thread() {
//	std::wwstringstream stream;
//	int result = 0;
//	while ((result = recv(s, buffer, BUFFER_SIZE, 0)) > 0) {
//		buffer[result] = 0;
//		stream << buffer;
//		for (int i = 0; i < result; i++)
//			if (buffer[i] == '\n') {
//				std::wstring str;
//				std::getline(stream, str);
//				recieve_new_line(str);
//			}
//	}
//}
