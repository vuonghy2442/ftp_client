// ftp_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ftp_client.h"

int main()
{
	_setmode(_fileno(stdout), _O_U16TEXT);

	std::wstring ftp_server;
	int port;
	std::wcout << "Enter server: ";
	std::getline(std::wcin, ftp_server);

	std::wcout << "Enter port: ";
	std::wcin >> port;

	std::wcin.ignore(INT_MAX, '\n');

	try {
		ftp_client client(ftp_server, port);
		std::wcout << "Successfully connect to host" << std::endl;

		client.send_command(L"OPTS UTF8 ON");

		std::wstring userName;
		std::wstring password;
		std::wstring localdir = L"D:\\";

		std::wcout << "Enter User Name: ";
		std::wcin >> userName;
		client.send_command(L"USER " + userName);
		std::wcout << "Enter password for user " << userName << ": ";
		std::wcin >> password;
		client.send_command(L"PASS " + password);

		while (1) {
			std::wstring command;
			std::wcout << "> ";
			std::getline(std::wcin, command);

			if (command == L"dir") {
				client.dir();
			}
			else if (command == L"passive") {
				client.to_mode(PASSIVE_MODE);
			}
			else if (command == L"active") {
				client.to_mode(ACTIVE_MODE);
			}
			else if (command == L"get") {
				std::wstring source;
				std::wstring dest;
				std::wcout << "Enter file on host (source file): ";
				std::getline(std::wcin, source);
				std::wcout << "Enter file on client (dest file): ";
				std::getline(std::wcin, dest);
				client.download(source, localdir +L"\\"+ dest);
			}

			else if (command == L"mget")
			{
				std::vector <std::wstring> source;
				std::vector <std::wstring> dest;
				source.resize(0);
				dest.resize(0);
				std::wstring sourceName, destName;
				std::wcout << "<Enter .. for both to quit>\n";
				while (1)
				{
					std::wcout << "Enter file on host (source file): ";
					std::getline(std::wcin, sourceName);
					std::wcout << "Enter file on client (dest file): ";
					std::getline(std::wcin, destName);
					if (sourceName != L".." && destName != L"..")
					{
						source.push_back(sourceName);
						dest.push_back(destName);
					}
					else break;
				}
				for (int i = 0; i < source.size(); i++)
					client.download(source[i], localdir + L"\\" + dest[i]);
			}
			else if (command == L"put") {
				std::wstring source;
				std::wstring dest;
				std::wcout << "Enter file to client (source file): ";
				std::getline(std::wcin, source);
				std::wcout << "Enter file on host (dest file): ";
				std::getline(std::wcin, dest);
				client.upload(localdir + L"\\" +source,  dest);
			}

			else if (command == L"mput")
			{
				std::vector <std::wstring> source;
				std::vector <std::wstring> dest;
				source.resize(0);
				dest.resize(0);
				std::wstring sourceName, destName;
				std::wcout << "<Enter .. for both to quit>\n";
				while (1)
				{
					std::wcout << "Enter file to client (source file): ";
					std::getline(std::wcin, sourceName);
					std::wcout << "Enter file on host (dest file): ";
					std::getline(std::wcin, destName);
					if (sourceName != L".." && destName != L"..")
					{
						source.push_back(sourceName);
						dest.push_back(destName);
					}
					else break;
				}
				for (int i = 0; i < source.size(); i++)
					client.upload(localdir + L"\\" + source[i],  dest[i]);
			}

			else if (command == L"delete") {
				std::wstring deleteFile;
				std::wcout << "Enter file name: ";
				std::getline(std::wcin, deleteFile);
				client.send_command(L"DELE " + deleteFile);
			}
			else if (command == L"mdelete")
			{
				std::vector <std::wstring> deleteFile;
				deleteFile.resize(0);
				std::wstring name;
				std::wcout << "<Enter .. to quit>\n";
				while (1)
				{
					std::wcout << "Enter file name: ";
					std::getline(std::wcin, name);
					if (name != L"..")
						deleteFile.push_back(name);
					else break;
				}
				for (int i = 0; i < deleteFile.size(); i++)
					client.send_command(L"DELE " + deleteFile[i]);
			}
			else if (command == L"cd")
			{
				std::wstring drtr;
				std::wcout << "Enter new directory: ";
				std::getline(std::wcin, drtr);
				client.send_command(L"CWD " + drtr);
			}
			else if (command == L"lcd")
			{
				std::wcout << "Enter new local directory: ";
				std::getline(std::wcin, localdir);
				DWORD ftyp = GetFileAttributesW(localdir.c_str());
				if (ftyp != 0xffffffff && (ftyp & FILE_ATTRIBUTE_DIRECTORY))
					std::wcout << L"Local directory now is " + localdir << std::endl;
				else
				{
					std::wcout << L"This is not a directory! Default local directory D:\\ will be used." << std::endl;
					localdir = L"D:\\";
				}
			}
			else if (command == L"pwd")
			{
				client.send_command(L"PWD");
			}
			else if (command == L"mkdir") {
				std::wstring folderName;
				std::wcout << "Enter new folder name: ";
				std::getline(std::wcin, folderName);
				client.send_command(L"MKD " + folderName);
			}
			else if (command == L"rmdir") {
				std::wstring folderName;
				std::wcout << "Enter folder name to delete: ";
				std::getline(std::wcin, folderName);
				client.send_command(L"RMD " + folderName);
			}

			else if (command == L"quit") {
				client.send_command(L"QUIT");
				return 0;
			}
			else {
				client.send_command(command);
			}
		}
	}
	catch (int e) {
		switch (e) {
		case WS_START_ERROR:
			std::wcout << "Cannot start winsock service" << std::endl;
			return 1;
		case RESOLVE_HOST_ERROR:
			std::wcout << "Cannot resolve the given hostname" << std::endl;
			return 1;
		case CREATE_SOCKET_ERROR:
			std::wcout << "Cannot create socket" << std::endl;
			return 1;
		case CONNECT_HOST_ERROR:
			std::wcout << "Cannot connect to host" << std::endl;
			return 1;
		}
	}
	return 0;
}

