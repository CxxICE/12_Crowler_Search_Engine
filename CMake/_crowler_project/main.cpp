#pragma comment (lib, "crypt32")

#include <iostream>
#include <functional>
#include <future>
#include <chrono>

#include <boost/asio/spawn.hpp>
#include <boost/url.hpp>

#include "thread_pool/thread_pool.h"
#include "crowler/crowler.h"
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
	int depth;
	std::string startPage;	
	try
	{		
		parser.setFile("crowler.ini");

		dbHost = parser.getValue<std::string>("DATABASE.host");
		dbPort = parser.getValue<std::string>("DATABASE.port");
		dbName = parser.getValue<std::string>("DATABASE.dbname");
		dbUser = parser.getValue<std::string>("DATABASE.user");
		dbPassword = parser.getValue<std::string>("DATABASE.password");

		depth = parser.getValue<int>("CROWLER.depth");
		startPage = parser.getValue<std::string>("CROWLER.startPage");		

		database = std::make_shared<CrowlerDatabase>(
			dbHost,
			dbPort,
			dbName,
			dbUser,
			dbPassword);
		//database->dropTables();
		database->createTables();

		Crowler crowler;
		crowler.setDepth(depth);
		crowler.setDatabase(database);
		crowler.setStartUrl(startPage);
		crowler.start();
		std::cout << "\x1B[32m" << "Crowler: started" << "\x1B[0m\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		while (crowler.isWorking()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		};

		std::cout << "\x1B[32m" << "Crowler: finished" << "\x1B[0m\n";
	}
	catch (const std::exception &ex)
	{
		std::cerr << "\x1B[31m" << ex.what() << "\x1B[0m\n";
	}
	getchar();
	return 0;
}