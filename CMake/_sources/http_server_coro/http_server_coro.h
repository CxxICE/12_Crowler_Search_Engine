#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/url.hpp>
#include <boost/locale.hpp>

#include <crowler_database/crowler_database.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class HttpServerCoro
{
public:
	HttpServerCoro() = default;
	void start(int port);
	void setDatabase(std::shared_ptr<CrowlerDatabase> database);

	HttpServerCoro(const HttpServerCoro &other) = delete;
	HttpServerCoro(HttpServerCoro &&other) = delete;
	HttpServerCoro &operator=(const HttpServerCoro &other) = delete;
	HttpServerCoro &operator=(HttpServerCoro &&other) = delete;

	~HttpServerCoro() = default;

private:
	std::shared_ptr<CrowlerDatabase> _database;	

	void acceptConnection(asio::yield_context yield, asio::io_context &ioc,
		std::string host, int port);
	void handleConnection(asio::yield_context yield, std::shared_ptr<tcp::socket> socket);

	void createResponseGet(
		http::request<http::dynamic_body> &request,
		http::response<http::dynamic_body> &response);
	void createResponsePost(
		http::request<http::dynamic_body> &request,
		http::response<http::dynamic_body> &response);
	void createResponseFailure(beast::multi_buffer &mb, std::string_view message);
	bool getContentFromFile(beast::multi_buffer &mb, std::string_view filename);
	std::string urlDecode(std::string_view str);
	void errorCodeHandle(beast::error_code ec, char const *describe);
};




