#pragma comment (lib, "crypt32")

#include <iostream>
#include <functional>
#include <future>
#include <chrono>

#include <boost/asio/spawn.hpp>
#include <boost/url.hpp>

#include "http_server_coro/http_server_coro.h"
#include "crowler_database/crowler_database.h"
#include "parser_ini_file/parser_ini_file.h"

#include <Windows.h>


int main() 
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	system("color");
	std::shared_ptr<CrowlerDatabase> database;
	ParserIniFile parser;
	std::string dbHost;
	std::string dbPort;
	std::string dbName;
	std::string dbUser;
	std::string dbPassword;	
	int port;
		
	try
	{
		parser.setFile("search_engine.ini");

		dbHost = parser.getValue<std::string>("DATABASE.host");
		dbPort = parser.getValue<std::string>("DATABASE.port");
		dbName = parser.getValue<std::string>("DATABASE.dbname");
		dbUser = parser.getValue<std::string>("DATABASE.user");
		dbPassword = parser.getValue<std::string>("DATABASE.password");		

		port = parser.getValue<int>("SERVER.port");

		database = std::make_shared<CrowlerDatabase>(
			dbHost,
			dbPort,
			dbName,
			dbUser,
			dbPassword);

		HttpServerCoro server;
		server.setDatabase(database);
		server.start(port);		
	}
	catch (const std::exception &ex)
	{
		std::cerr << "\x1B[31m" << ex.what() << "\x1B[0m\n";
	}	


	return 0;
}