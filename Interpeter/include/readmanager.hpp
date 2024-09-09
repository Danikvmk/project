#pragma once

#include <fstream>
#include <string>
#include <stdexcept>

class readManager {
public:
	readManager(const std::string&);

	std::string get();
private:
	std::streampos FilesSize(std::ifstream&);
	std::string buf;
	
	std::size_t offset;
};
