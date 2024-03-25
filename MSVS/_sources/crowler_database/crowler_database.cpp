#include "crowler_database.h"

CrowlerDatabase::CrowlerDatabase(
	const std::string &host,
	const std::string &port,
	const std::string &dbname,
	const std::string &user,
	const std::string &password)
{
	try
	{		
		_host = host;
		_port = port;
		_dbname = dbname;
		_user = user;
		_password = password;		
	}
	catch (const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "CrowlerDatabase: construction fail" << "\x1B[0m\n";
		_m.unlock();
	}	
}


bool CrowlerDatabase::createTables()
{
	try
	{
		std::unique_ptr<pqxx::connection> connection = createConnection();
		pqxx::work w{ *connection };
		w.exec(R"create(
				BEGIN;
				CREATE TABLE IF NOT EXISTS links(
				id_link BIGSERIAL PRIMARY KEY,
				link VARCHAR NOT NULL UNIQUE,
				link_description VARCHAR NOT NULL
				);

				CREATE TABLE IF NOT EXISTS words(
				id_word BIGSERIAL PRIMARY KEY,
				word VARCHAR NOT NULL UNIQUE
				);

				CREATE TABLE IF NOT EXISTS links_words_count(
				id_link BIGINT REFERENCES links(id_link) ON UPDATE CASCADE ON DELETE CASCADE NOT NULL,
				id_word BIGINT REFERENCES words(id_word) ON UPDATE CASCADE ON DELETE CASCADE NOT NULL,
				count_words INTEGER NOT NULL CHECK(count_words > 0),
				PRIMARY KEY(id_link, id_word)
				);
				COMMIT;
				)create");		
		w.commit();
	}
	catch (const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "CrowlerDatabase: createTables fail" << "\x1B[0m\n";
		_m.unlock();
		return true;
	}
	return false;
}

bool CrowlerDatabase::dropTables()
{
	try
	{
		std::unique_ptr<pqxx::connection> connection = createConnection();
		pqxx::work w{ *connection };
		w.exec(R"drop(
				BEGIN;
				DROP TABLE IF EXISTS links_words_count;
				DROP TABLE IF EXISTS links;
				DROP TABLE IF EXISTS words;
				COMMIT;
				)drop");
		w.commit();
	}
	catch (const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "CrowlerDatabase: dropTables fail" << "\x1B[0m\n";
		_m.unlock();
		return true;
	}
	return false;
}

bool CrowlerDatabase::addOnePageData(
	const std::string &link, 
	const std::string &linkDescription, 
	const std::unordered_map<std::string, int> &words)
{	
	int idLink = 0;
	int idWord = 0;
	try
	{
		std::unique_ptr<pqxx::connection> connection = createConnection();			
		connection->prepare("links_words_count_insert",			
			"INSERT INTO  links_words_count(id_link, id_word, count_words) "
			"VALUES($1, $2, $3) "
			"ON	CONFLICT (id_link, id_word) "
			//"DO NOTHING;");
			"DO UPDATE SET count_words = $3;");

		std::string linkEsc;
		std::string linkDescriptionEsc;
		{
			pqxx::work w{ *connection };
			linkEsc = w.esc(link);
			linkDescriptionEsc = w.esc(linkDescription);
			w.exec(
				"INSERT INTO links(link, link_description) "
				"VALUES ('" + linkEsc + "', '" + linkDescriptionEsc + "') "
				"ON CONFLICT (link) "
				"DO NOTHING;");
		
			idLink = w.query_value<int>(
				"SELECT id_link "
				"FROM links "
				"WHERE link = '" + linkEsc + "' FOR SHARE;");
			w.commit();
		}
		
		std::string wordEsc;
		for (const auto &word : words)
		{
			idWord = 0;
			{
				pqxx::work w{ *connection };
				wordEsc = w.esc(word.first);
				w.exec(
					"INSERT INTO words(word) "
					"VALUES ('" + wordEsc + "') "
					"ON CONFLICT (word) "
					"DO NOTHING;");
			
				idWord = w.query_value<int>(
					"SELECT id_word "
					"FROM words "
					"WHERE word = '" + wordEsc + "' FOR SHARE;");
				w.commit();
			}
			{
				pqxx::work w{ *connection };
				w.exec_prepared("links_words_count_insert", idLink, idWord, word.second);
				w.commit();
			}
			
		}	
		
	}
	catch (const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "CrowlerDatabase: addOnePageData fail" << "\x1B[0m\n";
		_m.unlock();
		return true;
	}
	return false;
}

searchResultType CrowlerDatabase::getLinksOnWordsRelevance(const std::vector<std::string> &words)
{
	searchResultType sRez;
	try
	{
		std::unique_ptr<pqxx::connection> connection = createConnection();
		pqxx::work w{ *connection };
		std::string wordsIn;
		for (int i = 0; i < words.size(); ++i)
		{
			if (i == 0)
			{
				wordsIn = "'%" + words[i] + "%'";
			}
			else
			{
				wordsIn = wordsIn + ", '%" + words[i] + "%'";
			}		
		}
		//поиск слов в БД
		//поиск засчитывает в том числе слова, которые содержат требуемую комбинацию букв, а не полное слово полностью
		//запрос возвращает список упорядоченный на основе суммарного кол-ва упоминаний каждого 
		//слова из запроса (список огрничен 50 словами)
		auto result = w.query<std::string, std::string>(
			"SELECT l.link, l.link_description "
			"FROM links_words_count AS lwc "
			"JOIN links AS l ON l.id_link = lwc.id_link "
			"JOIN words AS w ON w.id_word = lwc.id_word "
			//"WHERE word IN(" + wordsIn + ") "
			"WHERE word LIKE ANY (ARRAY[" + wordsIn + "]) "
			"GROUP BY l.link, l.link_description "
			"ORDER BY sum(lwc.count_words) DESC "
			"LIMIT 50;"
		);
		for (const auto &r : result)
		{
			sRez.push_back({ std::get<0>(r), std::get<1>(r) });
		}
	}
	catch (const std::exception &ex)
	{
		_m.lock();
		std::cerr << "\x1B[31m" << ex.what() << ": " << "CrowlerDatabase: getLinksOnWordsRelevance fail" << "\x1B[0m\n";		
		_m.unlock();
	}
	return sRez;
}

std::unique_ptr<pqxx::connection> CrowlerDatabase::createConnection()
{
	return std::make_unique<pqxx::connection>(
		"host=" + _host +
		" port=" + _port +
		" dbname=" + _dbname +
		" user=" + _user +
		" password=" + _password);	
}
