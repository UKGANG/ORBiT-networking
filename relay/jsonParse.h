
#include "./include/json.hpp"

#include <string>

class jsonParse
{
public:
	static nlohmann::json parseFile(std::string fileName); // read a jason file and return a json object
private:

};
