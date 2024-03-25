#pragma once

#include <iostream>
#include <tuple>
#include <mutex>
#include <memory>
#include <vector>
#include <unordered_map>

#include <pqxx/pqxx>

using searchResultType = std::vector<std::tuple<std::string, std::string>>;

class CrowlerDatabase
{
public:
	CrowlerDatabase(
		const std::string &host, 
		const std::string &port, 
		const std::string &dbname, 
		const std::string &user, 
		const std::string &password);

	CrowlerDatabase(const CrowlerDatabase &other) = delete;
	CrowlerDatabase(CrowlerDatabase &&other) = delete;
	CrowlerDatabase &operator=(CrowlerDatabase &&other) = delete;
	CrowlerDatabase &operator=(const CrowlerDatabase &other) = delete;	

	bool createTables();
	bool dropTables();
	bool addOnePageData(
		const std::string &link, 
		const std::string &linkDescription, 
		const std::unordered_map<std::string, int> &words);
	searchResultType getLinksOnWordsRelevance(
		const std::vector<std::string> &words);

	~CrowlerDatabase() = default;
private:		
	std::string _host;
	std::string _port;
	std::string _dbname;
	std::string _user;
	std::string _password;

	std::mutex _m;//создан для защиты вывода в консоль

	std::unique_ptr<pqxx::connection> createConnection();
};

