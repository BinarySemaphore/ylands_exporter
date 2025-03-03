#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <vector>

std::string string_join(const std::vector<std::string>& str_list, const char* delimiter) {
	std::ostringstream combined;
	if (str_list.size() == 1) return str_list[0];
	std::copy(
		str_list.begin(),
		str_list.end(),
		std::ostream_iterator<std::string>(combined, delimiter)
	);
	return combined.str();
}

std::vector<std::string> string_split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream str_iter(str);
	std::string token;
	while (std::getline(str_iter, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

std::string string_replace(const std::string& str, const char* target, const char* repl) {
	int start;
	int target_length = std::strlen(target);
	int repl_length = std::strlen(repl);
	std::string replaced = str;
	start = replaced.find(target);
	while (start != std::string::npos) {
		replaced = replaced.replace(start, target_length, repl);
		start = replaced.find(target, start + repl_length);
	}
	return replaced;
}

std::string f_base_filename_no_ext(const char* filename) {
	int start;
	int end;
	std::string base_filename;
	base_filename = filename;
	start = base_filename.rfind('/');
	if (start == std::string::npos) {
		start = base_filename.rfind('\\');
		if (start == std::string::npos) start = 0;
	}
	end = base_filename.rfind('.');
	if (end == std::string::npos) end = base_filename.size();
	return base_filename.substr(start, end - start);
}

std::string f_base_dir(const char* filename) {
	std::string base_dir = "";
	std::vector<std::string> dir_list;
	dir_list = string_split(filename, '/');
	if (dir_list.size() > 1) {
		dir_list = std::vector<std::string>(dir_list.begin(), dir_list.end() - 1);
		base_dir = string_join(dir_list, "/") + "/";
	} else {
		dir_list = string_split(filename, '\\');
		if (dir_list.size() > 1) {
			dir_list = std::vector<std::string>(dir_list.begin(), dir_list.end() - 1);
			base_dir = string_join(dir_list, "\\") + "\\";
		}
	}
	return base_dir;
}

class CustomException : public std::exception {
protected:
	std::string msg;
public:
	CustomException() {}
	const char* what() const noexcept override {
		return this->msg.c_str();
	}
};

class SaveException : public CustomException {
public:
	SaveException(std::string msg) {
		this->msg = msg;
	}
};

class LoadException : public CustomException {
public:
	LoadException(std::string msg) {
		this->msg = msg;
	}
};

class ParseException : public CustomException {
public:
	ParseException(std::string msg) {
		this->msg = msg;
	}
};

class AllocationException : public CustomException {
public:
	AllocationException() {}
	AllocationException(const char* who, int size) {
		this->msg = "Unable to allocate enough initial memory ("
		+ std::to_string(size) + ") for " + who;
	}
};

class ReallocException : public AllocationException {
public:
	ReallocException(const char* who, int size) {
		this->msg = "Unable to reallocate enough memory ("
		+ std::to_string(size) + ") for " + who;
	}
};

#endif // UTILS_H