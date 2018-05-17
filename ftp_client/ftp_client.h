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
#define HOST_REJECT_ERROR 6
#define DISCONECTED 7


#define POSITIVE_PRE 1
#define POSITIVE_COMP 2
#define POSITIVE_INM 3
#define NEGATIVE_TRANS 4
#define NEGATIVE_PERM 5

#define ACTIVE_MODE 1
#define PASSIVE_MODE 2

#define BUFFER_SIZE 100

struct server_response
{
	int code;
	std::wstring message;

	server_response(std::wstring message);

	bool is(int code);
	bool is_positive();
	bool is_negative();
};

class ftp_client
{
private:
	WSADATA SData;
	SOCKET s;
	addrinfoW *host = NULL;

	int con_mode = ACTIVE_MODE;

	std::wostream &os;

	int u_line = 0; //number of unprocessed line
	std::wstringstream response;

	char *buffer;

	bool readable(int wait_time);
	int recieve_info();

	server_response wait_response();

	std::wstring getline(bool force = false);
	SOCKET open_port();
	SOCKET passive_mode();
public:
	~ftp_client();

	ftp_client(std::wstring server, int port = 21, std::wostream & os = std::wcout);

	void print_response();


	void dir();
	void download(std::wstring source, std::wstring dest);
	void upload(std::wstring source, std::wstring dest);
	bool send_command(std::wstring command, bool get_res = true);
	SOCKET send_port_command(std::wstring command);
	void to_mode(int mode);
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
