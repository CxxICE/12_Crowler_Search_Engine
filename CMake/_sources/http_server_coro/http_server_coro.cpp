#include "http_server_coro.h"

void HttpServerCoro::start(int port)
{
	asio::io_context ioc{ 1 };
	asio::spawn(ioc, std::bind(
		&HttpServerCoro::acceptConnection, this,
		std::placeholders::_1,
		std::ref(ioc),
		std::string("0.0.0.0"),
		port),
		[](std::exception_ptr ex)
		{
			if (ex) std::rethrow_exception(ex);
		});
	ioc.run();
}

void HttpServerCoro::setDatabase(std::shared_ptr<CrowlerDatabase> database)
{
	_database = database;	
}

void HttpServerCoro::acceptConnection(
	asio::yield_context yield, asio::io_context &ioc,
	std::string host, int port)
{
	beast::error_code ec;
	asio::ip::address const address = asio::ip::make_address(host);
	tcp::endpoint ep(address, port);
	tcp::acceptor acceptor{ ioc, ep };
	std::cout << "\x1B[32m" << "Server: started at localhost" << ":" << port << "\x1B[0m\n";
	while (true)
	{
		std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(ioc);

		acceptor.async_accept(*socket, yield[ec]);
		if (ec) return errorCodeHandle(ec, "Server acceptConnection fail");

		asio::spawn(ioc, std::bind(
			&HttpServerCoro::handleConnection, this,
			std::placeholders::_1,
			socket),
			[](std::exception_ptr ex)
			{
				if (ex) std::rethrow_exception(ex);
			});
	}
}

void HttpServerCoro::handleConnection(asio::yield_context yield, std::shared_ptr<tcp::socket> socket)
{
	beast::error_code ec;
	beast::flat_buffer buf;
	http::request<http::dynamic_body> request;

	http::async_read(*socket, buf, request, yield[ec]);
	if (ec) return errorCodeHandle(ec, "Server read fail");

	http::response<http::dynamic_body> response;
	response.version(request.version());
	response.keep_alive(false);

	switch (request.method())
	{
	case http::verb::get:
		response.result(http::status::ok);
		response.set(http::field::server, "Beast");
		createResponseGet(request, response);
		break;
	case http::verb::post:
		response.result(http::status::ok);
		response.set(http::field::server, "Beast");
		createResponsePost(request, response);
		break;

	default:
		response.result(http::status::bad_request);
		response.set(http::field::content_type, "text/html");
		createResponseFailure(response.body(), "Invalid request-method '" + std::string(request.method_string()) + "'");
		break;
	}

	response.content_length(response.body().size());

	http::async_write(*socket, response, yield[ec]);
	if (ec) return errorCodeHandle(ec, "Server write fail");

	socket->shutdown(tcp::socket::shutdown_send, ec);
	if (ec && ec != beast::errc::not_connected)	return errorCodeHandle(ec, "Server socket shutdown fail");
}

void HttpServerCoro::createResponseGet(
	http::request<http::dynamic_body> &request,
	http::response<http::dynamic_body> &response)
{
	if (request.target() == "/")
	{
		if (getContentFromFile(response.body(), ".\\html_css\\search_page.html"))
		{
			response.set(http::field::content_type, "text/html");
		}
		else
		{
			response.result(http::status::internal_server_error);
		}
	}
	else if (request.target() == "/style.css")
	{
		if (getContentFromFile(response.body(), ".\\html_css\\style.css"))
		{
			response.set(http::field::content_type, "text/css");
		}
		else
		{
			response.result(http::status::internal_server_error);
		}
	}
	else
	{
		response.result(http::status::ok);
		response.set(http::field::content_type, "text/html");
		createResponseFailure(response.body(), "Page not found");
	}
}

void HttpServerCoro::createResponsePost(
	http::request<http::dynamic_body> &request,
	http::response<http::dynamic_body> &response)
{
	if (request.target() == "/")
	{
		std::string s = beast::buffers_to_string(request.body().data());

		std::cout << "POST data: " << s << std::endl;

		size_t pos = s.find('=');
		if (pos == std::string::npos)
		{
			response.result(http::status::bad_request);
			response.set(http::field::content_type, "text/html");
			createResponseFailure(response.body(), "Bad request");
			return;
		}

		std::string key = s.substr(0, pos);
		if (key != "search")
		{
			response.result(http::status::bad_request);
			response.set(http::field::content_type, "text/html");
			createResponseFailure(response.body(), "Bad request");
			return;
		}

		std::string value = s.substr(pos + 1);
		if (value.empty())
		{
			createResponseGet(request, response);
			return;
		}
		std::string valueDecoded = urlDecode(value);
		boost::locale::generator gen;
		std::locale loc = gen("ru_RU.UTF-8");
		std::string valueDecodedLower = boost::locale::to_lower(valueDecoded, loc);

		//очищаем текст от знаков пунктуации, пробельных символов и управл€ющих --> 
		// --> замена на простые пробелы во избежание соединени€ слов разделенных спецсимволами
		//перекодировка нужна дл€ корректного удалени€ символов по типу неразрывных пробелов и т.п.
		//без перекодировки функции std::ispunct std::isspace std::iscntrl не определ€ют подобные знаки
		std::basic_string<wchar_t> utf16string = boost::locale::conv::utf_to_utf<wchar_t, char>(valueDecodedLower);
		std::locale locSTD("ru_RU.UTF-8");
		std::for_each(utf16string.begin(), utf16string.end(),
			[&locSTD](wchar_t &ch)
			{
				if (std::ispunct(ch, locSTD) || std::isspace(ch, locSTD) || std::iscntrl(ch, locSTD))
				{
					ch = L' ';
				}
			});
		//обратное преобразование дл€ дальнейшей работы с utf8
		std::string utf8string = boost::locale::to_lower(
			boost::locale::conv::utf_to_utf<char, wchar_t>(utf16string),loc);		

		//разбиваем текст на слова с подсчетом их кол-ва
		std::istringstream stream;
		stream.imbue(std::locale("ru_RU.UTF-8"));
		stream.str(utf8string);
		std::vector<std::string> words;
		std::string word;
		while (stream >> word)
		{
			words.push_back(word);			
		}

		searchResultType searchResult;
		//запрос к Ѕƒ о наличии слов из строки поиска
		if (_database)
		{
			searchResult = _database->getLinksOnWordsRelevance(words);
		}
		else
		{
			throw std::exception("server database didn't specify");
		}

		
		if (!searchResult.empty())
		{
			bool successFileReading = getContentFromFile(response.body(), ".\\html_css\\result_page_1.html");
			beast::ostream(response.body())
				<< "Search results for \""
				<< valueDecoded
				<< "\"";
			successFileReading = getContentFromFile(response.body(), ".\\html_css\\result_page_2.html");
			for (const auto &url : searchResult)
			{
				beast::ostream(response.body())
					<< "<li><a href=\""
					<< std::get<0>(url) << "\">"
					<< std::get<1>(url) << "</a></li>";
			}
			successFileReading = getContentFromFile(response.body(), ".\\html_css\\result_page_3.html");

			if (!successFileReading)
			{
				response.result(http::status::internal_server_error);
			}
		}
		else
		{
			response.result(http::status::ok);
			response.set(http::field::content_type, "text/html");
			createResponseFailure(response.body(), "Your search \"" + valueDecoded + "\" - did not match any documents.");
		}
	}
	else
	{
		response.result(http::status::ok);
		response.set(http::field::content_type, "text/html");
		createResponseFailure(response.body(), "Page not found");
	}
}

void HttpServerCoro::createResponseFailure(beast::multi_buffer &mb, std::string_view message)
{
	mb.clear();
	beast::ostream(mb)
		<< "<html>\n"
		<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
		<< "<body>\n"
		<< "<center><br><br><br>\n"
		<< "<h1>"
		<< message.data()
		<< "</h1>\n"
		<< "</center>\n"
		<< "</body>\n"
		<< "</html>\n";
}

bool HttpServerCoro::getContentFromFile(beast::multi_buffer &mb, std::string_view filename)
{
	std::ifstream file;
	file.open(filename.data(), std::ios_base::in | std::ios_base::binary);
	if (file.is_open())
	{
		char readChar;
		while (file.get(readChar))
		{
			beast::ostream(mb) << readChar;
		}
		file.close();
		return true;
	}
	else
	{
		createResponseFailure(mb, "Internal server error");
		return false;
	}
}

std::string HttpServerCoro::urlDecode(std::string_view str)
{
	boost::urls::encoding_opts opts;
	opts.space_as_plus = true;
	boost::urls::decode_view dv(str, opts);
	return { dv.begin(), dv.end() };
}

void HttpServerCoro::errorCodeHandle(beast::error_code ec, char const *describe)
{
	std::cerr << "\x1B[31m" << describe << ": " << ec.message() << "\x1B[0m\n";
}