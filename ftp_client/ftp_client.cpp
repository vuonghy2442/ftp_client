#include "stdafx.h"
#include "ftp_client.h"

#include <codecvt>
#include <string>
#include <fstream>


// convert UTF-8 string to wstring
std::wstring utf8_to_wstring (const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string wstring_to_utf8 (const std::wstring& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

//milisecond
bool ftp_client::readable(int wait_time)
{
	fd_set s_set;
	s_set.fd_count = 1;
	s_set.fd_array[0] = s;

	timeval time;
	time.tv_sec = wait_time / 1000;
	time.tv_usec = wait_time % 1000;
	return select(0, &s_set, NULL, NULL, &time) > 0;
}

int ftp_client::recieve_info()
{
	do {
		int byte = recv(s, buffer, BUFFER_SIZE, 0);
		if (byte < 0) {
			return RECIEVE_SOCKET_ERROR;
		}
		else if (byte == 0) {
			return DISCONECTED;
		}

		buffer[byte] = 0;

		for (int i = 0; i < byte; i++)
			u_line += (buffer[i] == '\n'); //count number of unprocessed line

		response << buffer;
	} while (readable(10));

	return 0;
}

server_response ftp_client::wait_response()
{
	server_response res(L"200");
	while (1) {
		res = server_response(getline(true));
		if (res.is_positive() || res.is_negative())
			break;
	};
	print_response();
	return res;
}

std::wstring ftp_client::getline(bool force)
{
	if (readable(10) || force)
		recieve_info();

	if (u_line) {
		u_line--;
		std::wstring line;
		std::getline(response, line);
		os << line << std::endl;
		return line;
	}
	else
		return L"";
}

SOCKET ftp_client::open_port() {
	sockaddr_in add;
	socklen_t add_len = sizeof(add);
	getsockname(s, (sockaddr*)&add, &add_len);

	add.sin_port = 0; //use other port

	SOCKET data_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(data_s, (sockaddr*)&add, sizeof(add));

	getsockname(data_s, (sockaddr*)&add, &add_len);

	char ipAddress[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(add.sin_addr), ipAddress, INET_ADDRSTRLEN);

	listen(data_s, 10);

	if (!send_command(L"PORT " + std::to_wstring((add.sin_addr.s_addr >> 0) & 255) + L","
		+ std::to_wstring((add.sin_addr.s_addr >> 8) & 255) + L","
		+ std::to_wstring((add.sin_addr.s_addr >> 16) & 255) + L","
		+ std::to_wstring((add.sin_addr.s_addr >> 24) & 255) + L","
		+ std::to_wstring((add.sin_port) & 255) + L","
		+ std::to_wstring((add.sin_port >> 8) & 255))) {
		closesocket(data_s);
		data_s = INVALID_SOCKET;
	}

	return data_s;
}

ftp_client::~ftp_client() {
	closesocket(s);

	FreeAddrInfoW(host);
	WSACleanup();
	free(buffer);
}

ftp_client::ftp_client(std::wstring server, int port, std::wostream & os) : os(os)
{
	int iResult = WSAStartup(0x0202, &SData);
	if (iResult) throw WS_START_ERROR;

	//Resolving host name
	struct addrinfoW hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = GetAddrInfoW(server.c_str(), std::to_wstring(port).c_str(), &hints, &host);

	if (iResult) {
		FreeAddrInfoW(host);
		WSACleanup();
		throw RESOLVE_HOST_ERROR;
	}

	//Connect
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s == INVALID_SOCKET) {
		FreeAddrInfoW(host);
		WSACleanup();
		throw CREATE_SOCKET_ERROR;
	}

	iResult = connect(s, host->ai_addr, host->ai_addrlen);

	if (iResult) {
		FreeAddrInfoW(host);
		WSACleanup();
		throw CONNECT_HOST_ERROR;
	}

	buffer = (char*)malloc(BUFFER_SIZE + 1);

	if (wait_response().is_negative()) {
		closesocket(s);
		FreeAddrInfoW(host);
		WSACleanup();
		throw HOST_REJECT_ERROR;
	}
}

void ftp_client::print_response()
{
	while (u_line > 0)
		getline(false);
}

SOCKET ftp_client::passive_mode() {
	send_command(L"PASV", false);
	server_response res = wait_response();
	if (res.is_negative())
		return INVALID_SOCKET;

	size_t start = res.message.find(L'(');
	size_t end = res.message.find(L')');

	int ip1, ip2, ip3, ip4, p1, p2;
	wchar_t c;

	std::wstringstream port(res.message.substr(start + 1, end - start - 1));

	port >> ip1 >> c >> ip2 >> c >> ip3 >> c >> ip4 >> c >> p1 >> c >> p2;

	SOCKET data_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (data_s == INVALID_SOCKET)
		return INVALID_SOCKET;

	sockaddr_in add;
	add.sin_family = AF_INET;
	add.sin_addr.s_addr = ip1 | (ip2 << 8) | (ip3 << 16) | (ip4 << 24);
	add.sin_port = p1 | (p2 << 8);
	int iResult = connect(data_s, (sockaddr*)&add, sizeof(add));
	if (iResult)
		return INVALID_SOCKET;

	return data_s;
}

SOCKET ftp_client::send_port_command(std::wstring command) {
	if (con_mode == ACTIVE_MODE) {
		SOCKET data_s = open_port();
		if (data_s == INVALID_SOCKET)
			return data_s;

		if (send_command(command)) {
			SOCKET acc_s = accept(data_s, NULL, NULL);
			closesocket(data_s);
			return acc_s;
		}
		else
			return INVALID_SOCKET;
	}
	else {
		SOCKET data_s = passive_mode();
		if (data_s == INVALID_SOCKET)
			return data_s;

		if (send_command(command)) {
			return data_s;
		}
		else {
			closesocket(data_s);
			return INVALID_SOCKET;
		}
	}
}

void ftp_client::dir() {

	SOCKET data_s = send_port_command(L"LIST");

	if (data_s == INVALID_SOCKET)
		return;

	int len;
	while ((len = recv(data_s, buffer, BUFFER_SIZE, 0)) > 0) {
		buffer[len] = 0;
		std::wcout << utf8_to_wstring(buffer);
	}
	closesocket(data_s);
}

void ftp_client::upload(std::wstring source, std::wstring dest) {
	SOCKET data_s = send_port_command(L"STOR " + dest);

	if (data_s == INVALID_SOCKET)
		return;

	std::ifstream f;
	f.open(source, std::ios::in | std::ios::binary);

	while (f.read(buffer, BUFFER_SIZE))
		send(data_s, buffer, BUFFER_SIZE, 0);

	if (f.gcount())
		send(data_s, buffer, f.gcount(), 0);

	f.close();
	closesocket(data_s);
}

void ftp_client::download(std::wstring source, std::wstring dest) {
	SOCKET data_s = send_port_command(L"RETR " + source);

	if (data_s == INVALID_SOCKET)
		return;

	std::ofstream result;
	result.open(dest, std::ios::out | std::ios::binary);

	int len;
	while ((len = recv(data_s, buffer, BUFFER_SIZE, 0)) > 0) {
		result.write(buffer, len);
	}
	result.close();
	closesocket(data_s);
}

bool ftp_client::send_command(std::wstring command, bool get_res) {
	while (!command.empty() && (command.back() == '\n' || command.back() == '\r'))
		command.pop_back();

	if (command.empty())
		return true;

	command += L"\r\n";

	send(s, wstring_to_utf8(command).c_str(), command.length(), 0);
	
	if (!get_res)
		return true;

	return wait_response().is_positive();
}

void ftp_client::to_mode(int mode)
{
	if (mode != ACTIVE_MODE && mode != PASSIVE_MODE)
		return;

	this->con_mode = mode;
}

server_response::server_response(std::wstring message) : message(message)
{
	std::wstringstream s(message);
	s >> code;
}

bool server_response::is(int code)
{
	return this->code / 100 == code;
}

bool server_response::is_positive()
{
	return code / 100 <= 3;
}

bool server_response::is_negative()
{
	return code / 100 == 4 || code / 100 == 5;
}
