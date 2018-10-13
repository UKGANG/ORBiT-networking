
#include <iostream>
#include <string>

#include "jsonParse.h"
#include "./include/json.hpp"

using namespace std;
using json = nlohmann::json;


int main()
{
	jsonParse parser;

	cout << parser.parseFile("Config.json")["Log"] << endl;

	return(0);
}
