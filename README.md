# Дипломный проект «Поисковая система»

Поисковая система получает данные с сайтов, строит поисковые индексы и по запросу выдает релевантные результаты поисковой выдачи.

### Подпроекты в составе проекта

|Подпроект|Описание|
|:---:|:---|
|crowler|HTTP-клиент, парсит сайты и строит индексы исходя из частоты слов в документах|
|search_engine|HTTP-сервер, принимает запросы пользователя и возвращает результаты поиска|

### Основные составляющие проекта

|Класс|Описание|
|:---:|:---|
|`Crowler`|Осуществляет скачивание и индексацию html-cтраниц. Результаты индексации загружает в БД, все обнаруженные на странице ссылки добавляет в очередь пула потоков для последующей обработки. Глубина рекурсивной обработки вторичных ссылок задается пользователем<br> `Boost.Beast` `OpenSSL` `Boost.Locale` `Boost.URL` `std::regex` `std::sregex_token_iterator` `std::atomic` `std::mutex` `std::shared_ptr` `std::vector` `std::unordered_map` `std::string` `std::string_view` `std::stringstream` `std::for_each`|
|`CrowlerDatabase`|Класс для работы с базой данных PostgreSQL<br> `libpqxx` `std::mutex` `std::unique_ptr` `std::vector` `std::unordered_map` `std::string` `std::tuple`|
|`ThreadPool`|Пул потоков для обработки задач Crowler<br>`std::jthread` `std::stop_source` `std::latch` `std::function`|
|`SafeQueue`|Потокобезопасная очередь для хранения задач Crowler<br> `std::condition_variable` `std:atomic` `std::mutex` `std::unique_lock` `std::scoped_lock` `std:::queue` `std::chrono`|
|`HttpServerCoro`|Поисковый сервер выполняющий поиск в БД по запросам пользователей и вывод их на экран в порядке релевантности запросу<br> `Boost.Beast` `Boost.Coroutine` `Boost.Locale` `Boost.URL` `std::shared_ptr` `std::vector` `std::string` `std::string_view` `std::fstream` `std::stringstream` `std::bind`|
|`ParserIniFile`|Парсер ini-файлов для инициализации параметров Crowler, CrowlerDatabase, HttpServerCoro из соответствующих ini-файлов<br> `std::set` `std::fstream` `std::for_each` `std::is_same` `std::stof` `std::stod` `std::stold` `std::stoi` `std::stol` `std::stoul` `std::stoull`|

>[!NOTE]  
>Проект выполнен в двух вариантах:
>- Microsoft Visual Studio
>- CMake


>[!TIP]  
>Для сборки проекта требуется указать расположение заголовочных файлов и бинарников библиотеки Boost. 
>Остальные файлы библиотек собраны и приложены к проекту (libpqxx, OpenSSL, PostgreSQL).
