#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

std::string string_join(const std::vector<std::string>& str_list, const char* delimiter);

std::vector<std::string> string_split(const std::string& str, char delimiter);

std::string string_replace(const std::string& str, const char* target, const char* repl);

std::string f_base_filename_no_ext(const char* filename);

std::string f_base_dir(const char* filename);

class CustomException : public std::exception {
protected:
	std::string msg;
public:
	CustomException();
	const char* what() const noexcept override;
};

class SaveException : public CustomException {
public:
	SaveException(std::string msg);
};

class LoadException : public CustomException {
public:
	LoadException(std::string msg);
};

class ParseException : public CustomException {
public:
	ParseException(std::string msg);
};

class AllocationException : public CustomException {
public:
	AllocationException();
	AllocationException(const char* who, int size);
};

class ReallocException : public AllocationException {
public:
	ReallocException(const char* who, int size);
};

#endif // UTILS_H