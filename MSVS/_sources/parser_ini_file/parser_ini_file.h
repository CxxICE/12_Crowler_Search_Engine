#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <algorithm>
#include <exception>

class ParserIniFile
{
public:
	ParserIniFile() = default;
	
	void setFile(const std::string &filename);
	
	template<typename T>
	T getValue(const std::string &secVar);

	~ParserIniFile();
private:
	std::ifstream _file;
	std::string _filename;
	const char SECTION_OPEN = '[';
	const char SECTION_CLOSE = ']';
	const char COMMENT = ';';
	const char ASSIGN = '=';
	const char NEW_LINE = '\n';
	const char FLOAT_DIV = '.';
	const char DOT = '.';
	const char COMMA = ',';
	const char SPACES[3] = " \t";

	void cutSpaces(std::string &line);
};

template<typename T>
T ParserIniFile::getValue(const std::string &secVar)
{
	if (_file.is_open())
	{
		_file.seekg(0);
	}
	else
	{
		std::string message = "ParserIniFile: Не задан файл для инициализации";
		throw std::runtime_error(message);
	}
	T resultValue;
	std::string secName, varName, line, currentSec, currentVar;
	int lineCount = 0;
	bool notFindSec = true;
	bool notFindVar = true;
	std::set<std::string> variables;
	//контроль аргумента функции getValue() и разделение на section и value
	int posDot = secVar.find(DOT);
	if (posDot == std::string::npos)
	{
		std::string message = "ParserIniFile: Неверный формат аргумента \"" + secVar +
			"\" функции getValue(), наименования секции и переменной должны быть разделены точкой.";
		throw std::invalid_argument(message);
	}
	else
	{
		secName = secVar.substr(0, posDot);
		varName = secVar.substr(posDot + 1, secVar.length() - posDot);
		//перевод в нижний регистр для возможности регистронезависимого указания наименования секции
		std::for_each(secName.begin(), secName.end(), [](char &ch) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
		if (secName.length() == 0 || varName.length() == 0)
		{
			std::string message = "ParserIniFile: Неверный формат аргумента \"" + secVar +
				"\" функции getValue(), не указано наименование секции или переменной. \nКорректный формат: section.variable.";
			throw std::invalid_argument(message);
		}
		for (int i = 0; i < secName.length(); ++i)
		{
			char a = secName[i];
			if (std::ispunct(a) || std::iscntrl(a) || std::isspace(a))
			{
				std::string message = "ParserIniFile: В наименовании секции \"" + secName + "\" аргумента \"" + secVar +
					"\" функции getValue() содержатся недопустимые символы.";
				throw std::invalid_argument(message);
			}
		}
		for (int i = 0; i < varName.length(); ++i)
		{
			char a = varName[i];
			if (std::ispunct(a) || std::iscntrl(a) || std::isspace(a))
			{
				std::string message = "ParserIniFile: В наименовании переменной \"" + varName + "\" аргумента \"" + secVar +
					"\" функции getValue() содержатся недопустимые символы.";
				throw std::invalid_argument(message);
			}
		}
	}
	//формирование строки без пробелов, комментариев и командных символов
	while (!_file.eof())
	{
		line.clear();

		std::getline(_file, line);
		lineCount++;
		//удаление комментариев в строке
		size_t pos_comment = line.find(COMMENT);
		if (pos_comment != std::string::npos)
		{
			line.erase(pos_comment, line.length());
		}
		//удаление незначащих пробелов в начале и конце наименования секции
		cutSpaces(line);
		if (line.empty())
		{
			continue;
		}
		//поиск символов открытия и закрытия секции []
		size_t posOpen = line.find(SECTION_OPEN);
		size_t posClose = line.find(SECTION_CLOSE);

		if (posOpen != std::string::npos || posClose != std::string::npos)//в строке есть хотя бы одна скобка [] - вероятно это заголовок секции
		{
			if (posOpen != std::string::npos && posClose != std::string::npos && posOpen < posClose)//найдены обе скобки в строке и они стоят в правильном порядке
			{
				currentSec = line.substr(posOpen + 1, posClose - posOpen - 1);
				//удаление незначащих пробелов в начале и конце наименования секции, т.е. будет допустимо [  section1  ]
				cutSpaces(currentSec);
				//перевод в нижний регистр для возможности регистронезависимого указания наименования секции
				std::for_each(currentSec.begin(), currentSec.end(), [](char &ch) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
				if (currentSec == secName)
				{
					notFindSec = false;//секция найдена			
				}
				else
				{
					continue;
				}
			}
			else if (posOpen != std::string::npos && posClose == std::string::npos)//есть только скобка ]
			{
				std::string message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
					". Нет закрывающей скобки ].";
				throw std::runtime_error(message);
			}
			else if (posOpen == std::string::npos && posClose != std::string::npos)//есть только скобка [
			{
				std::string message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
					". Нет открывающей скобки [.";
				throw std::runtime_error(message);
			}
			else if (posOpen > posClose)//скобки в неправильном порядке ][
			{
				std::string message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
					". Скобки ][ в неправильном порядке.";
				throw std::runtime_error(message);
			}
		}
		else if (currentSec == secName)//находимся внутри секции
		{
			size_t posAssign = line.find(ASSIGN);
			currentVar = line.substr(0, posAssign);
			//удаление незначащих пробелов в начале и конце наименования переменной
			cutSpaces(currentVar);
			variables.insert(currentVar);
			if (currentVar == varName)
			{
				if ((currentVar.length() == line.length() - 1) || (posAssign == std::string::npos))//после = ничего не написано или знака = нет в строке с нужной переменной
				{
					std::string message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
						". Не задано значение переменной \"" + currentVar + "\"";
					throw std::runtime_error(message);
				}
				std::string valueString = line.substr(posAssign + 1, line.length() - posAssign);
				//удаление незначащих пробелов в начале и конце значения переменной, предполагается, что пробелы по краям не имеют значения для тектовых переменных
				cutSpaces(valueString);
				notFindVar = false;
				size_t posErr = 0;
				if constexpr (std::is_floating_point<T>::value)
				{
					std::string prevLoc = std::setlocale(LC_ALL, nullptr);//запись текущей локали
					try
					{
						//атоматический выбор локали для std::stold в зависимости от поля FLOAT_DIV
						if (FLOAT_DIV == DOT)
						{
							setlocale(LC_NUMERIC, "C");
						}
						else if (FLOAT_DIV == COMMA)
						{
							setlocale(LC_NUMERIC, "RU");
						}
						if constexpr (std::is_same<T, float>::value)
						{
							resultValue = std::stof(valueString, &posErr);
						}
						else if constexpr (std::is_same<T, double>::value)
						{
							resultValue = std::stod(valueString, &posErr);
						}
						else if constexpr (std::is_same<T, long double>::value)
						{
							resultValue = std::stold(valueString, &posErr);
						}
						std::setlocale(LC_ALL, prevLoc.c_str());//возврат локали при успешном преобразовании
						if (posErr != valueString.length())//преобразовалась не вся строка после знака =
						{
							std::string message;
							if (FLOAT_DIV == DOT && valueString.find(COMMA) != std::string::npos)//если разделителем считается точка, а найдена запятая
							{
								message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
									". Некорректное значение переменной \"" + currentVar + " = " + valueString + "\" - в качестве разделителя должна быть указана точка \".\".";
							}
							else if (FLOAT_DIV == COMMA && valueString.find(DOT) != std::string::npos)//если разделителем считается запятая, а найдена точка
							{
								message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
									". Некорректное значение переменной \"" + currentVar + " = " + valueString + "\" - в качестве разделителя должна быть указана запятая \",\".";
							}
							else
							{
								message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
									". Некорректное значение переменной \"" + currentVar + " = " + valueString + "\" - не соответсвует типу c плавающей точкой.";
							}
							throw std::runtime_error(message);
						}
					}
					catch (std::invalid_argument)
					{
						std::setlocale(LC_ALL, prevLoc.c_str());//возврат локали при неуспешном преобразовании
						std::string message = "ParserIniFile: Ошибка конвертирования строки \"" + valueString + "\" в число c плавающей точкой. std::invalid_argument";
						throw (std::invalid_argument(message));
					}
					catch (std::out_of_range)
					{
						std::setlocale(LC_ALL, prevLoc.c_str());//возврат локали при неуспешном преобразовании
						std::string message = "ParserIniFile: Ошибка конвертирования строки \"" + valueString + "\" в число c плавающей точкой. std::out_of_range";
						throw (std::out_of_range(message));
					}
				}
				else if constexpr (std::is_same<T, int>::value || std::is_same<T, long>::value || std::is_same<T, long long>::value ||
					std::is_same<T, unsigned long>::value || std::is_same<T, unsigned long long>::value)
				{
					try
					{
						if constexpr (std::is_same<T, int>::value)
						{
							resultValue = std::stoi(valueString, &posErr);
						}
						else if constexpr (std::is_same<T, long>::value)
						{
							resultValue = std::stol(valueString, &posErr);
						}
						else if constexpr (std::is_same<T, long long>::value)
						{
							resultValue = std::stoll(valueString, &posErr);
						}
						else if constexpr (std::is_same<T, unsigned long>::value)
						{
							resultValue = std::stoul(valueString, &posErr);
						}
						else if constexpr (std::is_same<T, unsigned long long>::value)
						{
							resultValue = std::stoull(valueString, &posErr);
						}
						if (posErr != valueString.length())//преобразовалась не вся строка после знака =
						{
							std::string message = "ParserIniFile: Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(lineCount) +
								". Некорректное значение переменной \"" + currentVar + " = " + valueString + "\" - не соответсвует целому типу.";
							throw std::runtime_error(message);
						}
					}
					catch (std::invalid_argument)
					{
						std::string message = "ParserIniFile: Ошибка конвертирования строки \"" + valueString + "\" в целое число. std::invalid_argument";
						throw std::invalid_argument(message);
					}
					catch (std::out_of_range)
					{
						std::string message = "ParserIniFile: Ошибка конвертирования строки \"" + valueString + "\" в целое число. std::out_of_range";
						throw std::out_of_range(message);
					}
				}
				else
				{
					resultValue = line.substr(posAssign + 1, line.length() - posAssign);
				}
			}
		}
	}
	if (notFindSec)
	{
		std::string message = "ParserIniFile: Искомая секция \"" + secName + "\" не найдена.";
		throw std::runtime_error(message);
	}
	else if (notFindVar)
	{
		std::string message = "ParserIniFile: Искомая переменная \"" + varName + "\" не найдена в секции \"" + secName + "\".\nНайдены следующие переменные:\n";
		for (const auto &var : variables)
		{
			message = message + var + '\n';
		}
		throw std::out_of_range(message);
	}
	return resultValue;
};




