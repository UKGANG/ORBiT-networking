
#include "jsonParse.h"

#include <fstream>

using namespace std;
using json = nlohmann::json;

json jsonParse::parseFile(string fileName)
{
	ifstream file;
	file.open(fileName);
	if(!file.is_open())
	{
		return(-1);
	}
	
	json parser;

	string fileString;
	while(file.good())
	{
		string tempLine;
		getline(file, tempLine);
		for(int i = 0; i < tempLine.length()-1 && tempLine.length() != 0 ; i++)
		{
			if(tempLine[i] == '/' && tempLine[i+1] == '/')
			{
				tempLine.erase(i, tempLine.length() - i);
				break;
			}
		}
		fileString += tempLine;
	}
	file.close();

	parser = json::parse(fileString);
	return(parser);
}
