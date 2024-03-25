#include "crowler.h"

Crowler::Crowler()
{	
	try
	{
		_working.store(0);
		_depth = 1;
		_rxRef.assign("<a href=\"([^\"]*)\"", std::regex_constants::optimize);
		_rxTitle.assign("<title>([^<]*)</title>", std::regex_constants::optimize);
		_rxTag.assign("(<[^]*?>)", std::regex_constants::optimize);
		_rxScript.assign("(<script[^]*?</script)|(<style[^]*?</style)", std::regex_constants::optimize);
	}
	catch (const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "Crowler: construction fail" << "\x1B[0m\n";
		_m.unlock();
	}	
}

void Crowler::setStartUrl(std::string startUrl)
{
	_startUrl = startUrl;
}

void Crowler::setDepth(int depth)
{
	if (depth > 0)
	{
		_depth = depth;
	}
	else
	{
		throw std::out_of_range("attempt to set depth <= 0");
	}
}

void Crowler::setDatabase(std::shared_ptr<CrowlerDatabase> database)
{
	_database = database;	
}

void Crowler::start()
{		
	_pool.submit(std::bind(&Crowler::parseOnePage, this, _startUrl, _depth));
}

bool Crowler::isWorking()
{
	return _working.load() > 0;
}

void Crowler::parseOnePage(std::string url, int depth)
{	
	try
	{
		int wTh = _working.fetch_add(1);
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[34m" << "Thread: "
			<< std::this_thread::get_id()
			<< " beginWork. Working "
			<< wTh + 1 << " threads\n"			
			<< url << "\x1B[0m\n";
		_m.unlock();
#endif
		boost::locale::generator gen;
		std::locale loc = gen("ru_RU.UTF-8");
		boost::url_view urlView(url);
		std::string scheme = urlView.scheme();
		std::string host = boost::locale::to_lower(urlView.host(), loc);

		std::string page = getHtmlContent(url);		
		if (page.empty())
		{
			wTh = _working.fetch_sub(1);
#ifdef CROWLER_COUT
			_m.lock();
			std::cout << "\x1B[34m" << "Thread: "
				<< std::this_thread::get_id()
				<< " finishWork. Working "
				<< wTh - 1 << " threads"
				<< "\x1B[0m\n";
			_m.unlock();
#endif
			return;
		}
		//получаем описание для текущей ссылки
		std::smatch m;
		std::regex_search(page, m, _rxTitle);
		std::string linkDescription = m[1];

		//поиск новых ссылок на странице, если не достигли максимальной глубины индексации страниц
		if (depth > 1)
		{
			//получаем перечень ссылок для их последующей индексации
			std::vector<std::string> links;
			std::sregex_token_iterator it{ std::begin(page), std::end(page), _rxRef, {1} };
			std::sregex_token_iterator it_end{};
			while (it != it_end)
			{
				std::string link = *it++;
				if (it == it_end) break;
				std::string description = *it++;
				if (!link.empty() && link.front() != '#')
				{
					if (link.substr(0, 4) == "http")//абсолютная ссылка
					{
						links.push_back(link);
					}
					else if (link.substr(0, 2) == "//")//абсолютная ссылка
					{
						links.push_back(scheme + ":" + link);
					}
					else if (link.substr(0, 3) == "../")//относительная ссылка
					{
						auto first = link.find("../");
						auto last = link.rfind("../");
						int count = (last - first + 3)/3;
						link.erase(0, count * 3);
						std::string urlCopy = url;
						for (int i = 0; i < count + 1; ++i)
						{
							urlCopy.erase(urlCopy.rfind("/"));
						}						
						links.push_back(urlCopy + "/" + link);
					}
					else if (link.substr(0, 3) == "./")//относительная ссылка
					{
						link.erase(0, 2);
						std::string urlCopy = url;
						urlCopy.erase(urlCopy.rfind("/"));
						links.push_back(urlCopy + '/' + link);
					}
					else
					{
						if (link.front() == '/')//ссылка относительно корня сайта
						{
							links.push_back(scheme + "://" + host + link);
						}
						else
						{
							links.push_back(url + link);//прочие относительные ссылки
						}
					}
				}
			}

			//отправляем все новые ссылки для обработки в thread pool	
			for (const auto &link : links)
			{
				_pool.submit(std::bind(&Crowler::parseOnePage, this, link, depth - 1));
			}			
		}

		//очищаем страницу от Script, Tag, Style
		std::string pageWoScript = std::regex_replace(page, _rxScript, " ");
		std::string pageWoTag = std::regex_replace(pageWoScript, _rxTag, " ");

		//очищаем текст от знаков пунктуации, пробельных символов и управляющих --> 
		// --> замена на простые пробелы во избежание соединения слов разделенных спецсимволами
		//перекодировка нужна для корректного удаления символов по типу неразрывных пробелов и т.п.
		//без перекодировки функции std::ispunct std::isspace std::iscntrl не определяют подобные знаки
		std::basic_string<wchar_t> utf16string = boost::locale::conv::utf_to_utf<wchar_t, char>(pageWoTag);
		std::locale locSTD("ru_RU.UTF-8");
		std::for_each(utf16string.begin(), utf16string.end(),
			[&locSTD](wchar_t &ch)
			{
				if (std::ispunct(ch, locSTD) || std::isspace(ch, locSTD) || std::iscntrl(ch, locSTD))
				{
					ch = L' ';
				}
			});
		//обратное преобразование для дальнейшей работы с utf8
		std::string utf8string = boost::locale::to_lower(
			boost::locale::conv::utf_to_utf<char, wchar_t>(utf16string),loc);		

		//разбиваем текст на слова с подсчетом их кол-ва
		std::istringstream stream;
		stream.imbue(std::locale("ru_RU.UTF-8"));
		stream.str(utf8string);
		std::unordered_map<std::string, int> words;
		std::string word;
		while (stream >> word)
		{
			if (word.length() > 2)
			{
				if (words.count(word))
				{
					words[word] += 1;
				}
				else
				{
					words.insert({ word, 1 });
				}
			}
		}
		if (_database)
		{			
			_database->addOnePageData(url, linkDescription, words);
		}
		else
		{
			throw std::exception("crowler database didn't specify");
		}	
	}
	catch (const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "Crowler: parseOnePage error\n" << url << "\x1B[0m\n";
		_m.unlock();
	}
	catch (...)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << "Crowler: parseOnePage unknown error\n" << url << "\x1B[0m\n";
		_m.unlock();
	}
	int wTh = _working.fetch_sub(1);
#ifdef CROWLER_COUT
	_m.lock();
	std::cout << "\x1B[34m" << "Thread: "
		<< std::this_thread::get_id()
		<< " finishWork. Working " 
		<< wTh - 1 << " threads"
		<< "\x1B[0m\n";
	_m.unlock();
#endif
	return;
}

std::string Crowler::getHtmlContent(std::string_view url)
{
	std::string result;
	try
	{
		boost::locale::generator gen;
		std::locale loc = gen("ru_RU.UTF-8");
		std::string country = std::use_facet<boost::locale::info>(loc).country();
		std::string encoding = std::use_facet<boost::locale::info>(loc).encoding();
		boost::url_view urlView(url);
		std::string scheme = urlView.scheme();
		std::string host = urlView.host();
		std::string path = urlView.path();
		
		asio::io_context ioc;		
		beast::flat_buffer buffer;			
		http::response<http::dynamic_body> res;
		beast::error_code ec;		

		if (scheme == "https")
		{			
			ssl::context ctx(ssl::context::tlsv12_client);
			ctx.set_default_verify_paths();			
			
			beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
			
			stream.set_verify_mode(ssl::verify_none);

			if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
			{
				beast::error_code ec{ static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category() };
				throw beast::system_error{ ec };
			}
			
			tcp::resolver resolver(ioc);
			auto resolved = resolver.resolve(host, scheme);
			beast::get_lowest_layer(stream).connect(resolved);
			beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
						
			http::request<http::empty_body> req{ http::verb::get, path, 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
						
			stream.handshake(ssl::stream_base::client);
			http::write(stream, req);
			http::read(stream, buffer, res);			
			get_lowest_layer(stream).cancel();
			stream.shutdown(ec);		
		}
		else
		{
			tcp::resolver resolver(ioc);
			beast::tcp_stream stream(ioc);

			auto const resolved = resolver.resolve(host, scheme);

			stream.connect(resolved);

			http::request<http::string_body> req{ http::verb::get, path, 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			http::write(stream, req);
			http::read(stream, buffer, res);			
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);			
		}
		
		result = httpCodeHandle(url, res);
				
		if (scheme == "https")
		{
			//https://www.boost.org/doc/libs/master/libs/beast/doc/html/beast/using_io/timeouts.html
			if (ec && ec != asio::error::eof && ec != asio::ssl::error::stream_truncated)
			{
				throw beast::system_error{ ec };
			}
		}
		else
		{
			if (ec && ec != beast::errc::not_connected)
			{
				throw beast::system_error{ ec };
			}
		}

		if (result.empty())
		{
#ifdef CROWLER_COUT
			_m.lock();
			std::cout << "\x1B[33m" << "Crowler: getHtmlContent failed for page:\n" << url << "\x1B[0m\n";
			_m.unlock();
#endif
		}
		else
		{
#ifdef CROWLER_COUT
			_m.lock();
			std::cout << "\x1B[32m" << "Crowler: getHtmlContent success for page:\n" << url << "\x1B[0m\n";
			_m.unlock();			
#endif
		}		
	}
	catch(const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "Crowler getHtmlContent error\n" << url << "\x1B[0m\n";
		_m.unlock();
	}
	catch (...)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << "Crowler: getHtmlContent unknown error\n" << url << "\x1B[0m\n";
		_m.unlock();
	}	
	return result;
}

std::string Crowler::httpCodeHandle(
	std::string_view url,
	const http::response<http::dynamic_body> &res)
{
	int code = res.base().result_int();
	std::string result;
	switch (code)
	{
	case 200://только этот код будет парситься
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[32m" << "\nCode 200\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif	
		result = getTextFromResponse(url, res);
		break;
	case 300:
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[35m" << "\nCode 300. Redirect from:\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif
		result = getHtmlContent(res.base()["Location"]);
		break;
	case 301:
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[35m" << "\nCode 301. Redirect from:\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif
		result = getHtmlContent(res.base()["Location"]);
		break;
	case 302:
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[35m" << "\nCode 302. Redirect from:\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif
		result = getHtmlContent(res.base()["Location"]);
		break;
	case 303:
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[35m" << "\nCode 303. Redirect from:\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif
		result = getHtmlContent(res.base()["Location"]);
		break;
	case 307:
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[35m" << "\nCode 307. Redirect from:\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif
		result = getHtmlContent(res.base()["Location"]);
		break;
	default:
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[33m" << "\nUnexpected code " << code << "\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif			
		break;
	}
	return result;
}

std::string Crowler::getTextFromResponse(std::string_view url,
	const http::response<http::dynamic_body> &res)
{
	std::string result;
	//упрощенный анализ на тип файла, 1я итерация проверка на null, 
	// из того что ее прошло проверяем на html, 
	//чтобы наверняка не распарсить что-то неизвестное
	if (notContainNull(res.body().data()))
	{
		result = buffers_to_string(res.body().data());
		std::string::size_type pos = result.find("html");
		if (std::string::npos == pos)
		{
			result.clear();
#ifdef CROWLER_COUT
			_m.lock();
			std::cout << "\x1B[33m" << "Crowler: Not html page. Ignore:\n" << url << "\x1B[0m\n";
			_m.unlock();
#endif
		}
	}
	else
	{
#ifdef CROWLER_COUT
		_m.lock();
		std::cout << "\x1B[33m" << "Crowler: Not a text page. Ignore:\n" << url << "\x1B[0m\n";
		_m.unlock();
#endif
	}
	return result;
}

bool Crowler::notContainNull(const boost::beast::multi_buffer::const_buffers_type &b)
{
	for (auto itr = b.begin(); itr != b.end(); itr++)
	{
		for (int i = 0; i < (*itr).size(); i++)
		{
			auto a = *((const char *)(*itr).data() + i);
			if (*((const char *)(*itr).data() + i) == 0)
			{
				return false;
			}
		}
	}
	return true;
}