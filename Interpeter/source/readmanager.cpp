#include "readmanager.hpp"

readManager::readManager(const std::string& input) {
	std::ifstream file(input);
	if (!file) {
		throw std::runtime_error("file is not opened");
	}

	auto size = FilesSize(file);
	buf.resize(size);
	if (!(file.read(&buf[0], size))) {
		throw std::runtime_error("reading error\n");
	}
	file.close();
	offset = 0;
}

std::string readManager::get() {
	return buf;
}


std::streampos readManager::FilesSize(std::ifstream& file) {
	file.seekg(0, std::ios::end);
	std::streampos size = file.tellg();
	file.seekg(0, std::ios::beg);
	return size;
}
