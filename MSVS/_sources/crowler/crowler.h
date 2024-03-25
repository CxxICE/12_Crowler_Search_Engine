#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <regex>
#include <cwctype>
#include <memory>
#include <atomic>

#include <boost/url.hpp>
#include <boost/locale.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>

#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <thread_pool/thread_pool.h>
#include <crowler_database/crowler_database.h>

#define CROWLER_COUT

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class Crowler
{
public:
	Crowler();
	Crowler(const Crowler &other) = delete;
	Crowler(Crowler &&other) = delete;
	Crowler &operator=(const Crowler &other) = delete;
	Crowler &operator=(Crowler &&other) = delete;
	
	void setStartUrl(std::string startUrl);
	void setDepth(int depth);
	void setDatabase(std::shared_ptr<CrowlerDatabase> database);
	void start();
	bool isWorking();

private:
	void parseOnePage(std::string url, int depth);
	std::string getHtmlContent(std::string_view url);
	bool notContainNull(const boost::beast::multi_buffer::const_buffers_type &b);
	std::string httpCodeHandle(
		std::string_view url,
		const http::response<http::dynamic_body> &res);
	std::string getTextFromResponse(
		std::string_view url,
		const http::response<http::dynamic_body> &res);

	std::string _startUrl;
	int _depth;	
	std::regex _rxRef;
	std::regex _rxTitle;
	std::regex _rxTag;
	std::regex _rxScript;
	std::atomic<int> _working;
	
	std::shared_ptr<CrowlerDatabase> _database;	

	std::mutex _m;//создан для защиты вывода в консоль

	//_pool д.б. последним, чтобы удержать другие поля 
	// от преждевременного уничтожения, пока _pool еще в работе
	ThreadPool _pool;
};