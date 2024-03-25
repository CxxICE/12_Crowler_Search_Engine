#include "parser_ini_file.h"

void ParserIniFile::setFile(const std::string &filename)
{
	try 
	{
		if (_file.is_open())
		{
			_file.close();
		}
		_filename = filename;
		_file.exceptions(std::ios::failbit | std::ios::badbit);
		_file.open(filename);				
	}
	catch (const std::ios::failure &ex)
	{
		std::string message = "ParserIniFile: Ошибка открытия файла: " + filename + "\nКод ошибки: " + std::to_string(ex.code().value());
		throw std::runtime_error(message);
	}
	catch (const std::exception &ex)
	{
		std::string message = std::to_string(*ex.what());
		throw std::runtime_error(message);
	}
};

ParserIniFile::~ParserIniFile()
{
	if(_file.is_open())
	{
		_file.close();	
	}
	
};

void ParserIniFile::cutSpaces(std::string &line)
{
	size_t posFirstSymbol = line.find_first_not_of(SPACES, 0);
	if (posFirstSymbol != std::string::npos)
	{
		line.erase(0, posFirstSymbol);
	}
	size_t posLastSymbol = line.find_last_not_of(SPACES, line.length() - 1);
	if (posLastSymbol != std::string::npos)
	{
		line.erase(posLastSymbol + 1, line.length());
	}
	if (line.length() > 0 && posFirstSymbol == std::string::npos && posLastSymbol == std::string::npos)//в строке только пробелы
	{
		line.clear();
	}
};