
--удаление всех таблиц бд
BEGIN;
DROP TABLE IF EXISTS links_words_count;
DROP TABLE IF EXISTS links;
DROP TABLE IF EXISTS words;
COMMIT;

--создание таблиц бд
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

--вставка данных
INSERT INTO links(link, link_description)
VALUES ('https://wiki.com/','wiki'), ('https://wiki2.com/','wiki2')
RETURNING id_link;

INSERT INTO links(link, link_description)
VALUES ('https://wiki.com5/','wiki')
RETURNING id_link;
  
INSERT INTO links(link, link_description) 
VALUES ('https://wiki.com/','wiki') 
ON CONFLICT (link) DO UPDATE SET link_description = 'wiki'
RETURNING id_link;

INSERT INTO words(word)
VALUES ('word797')
ON CONFLICT (word) 
DO NOTHING;

INSERT INTO words(word)
VALUES ('word5'), ('word2'), ('word3'), ('word4'), ('word5'), ('word6'), ('word7')
RETURNING id_word;

INSERT INTO  links_words_count(id_link, id_word, count_words)
VALUES(1,1,1),(1,2,2),(1,3,3),(1,4,4),(2,4,4),(2,5,5),(2,6,6),(2,7,7);

INSERT INTO  links_words_count(id_link, id_word, count_words)
VALUES(1,1,1)
ON CONFLICT (id_link, id_word) DO UPDATE SET count_words = 5;

--получение данных о конкретном слове
SELECT l.link, l.link_description, lwc.count_words
FROM links_words_count AS lwc
JOIN links AS l ON l.id_link = lwc.id_link
JOIN words AS w ON w.id_word = lwc.id_word
WHERE word = 'access';

--поиск слова
SELECT id_word
FROM words 
WHERE word = 'access';

--поиск ссылки
SELECT id_link
FROM links 
WHERE link = 'access';

--получение данных о конкретной ссылке
SELECT l.link, l.link_description, w.word, lwc.count_words
FROM links_words_count AS lwc
JOIN links AS l ON l.id_link = lwc.id_link
JOIN words AS w ON w.id_word = lwc.id_word
WHERE link = 'https://developer.wikimedia.org';

--вывод таблицы links
SELECT *
FROM links;

--вывод таблицы words
SELECT *
FROM words;

--получение данных о кол-ве раз которое встретились слова из набора на указанной url
SELECT l.link, l.link_description, sum(lwc.count_words)
FROM links_words_count AS lwc
JOIN links AS l ON l.id_link = lwc.id_link
JOIN words AS w ON w.id_word = lwc.id_word
WHERE word LIKE ANY (ARRAY['%wiki%','%word2%','%word4%'])
GROUP BY l.link, l.link_description
ORDER BY sum(lwc.count_words) DESC
LIMIT 50;

--вывод всей БД
SELECT l.link, l.link_description, w.word, lwc.count_words
FROM links_words_count AS lwc
JOIN links AS l ON l.id_link = lwc.id_link
JOIN words AS w ON w.id_word = lwc.id_word;

--кол-во записей
SELECT count(l.link)
FROM links_words_count AS lwc
JOIN links AS l ON l.id_link = lwc.id_link
JOIN words AS w ON w.id_word = lwc.id_word;

