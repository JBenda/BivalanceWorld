#pragma once

#include <filesystem>

constexpr int EVENT_SIZE = sizeof(inotify_event);
constexpr int BUF_LEN = 1024 * (EVENT_SIZE + 16);
inline const char* fetchFilename(int argc, const char** argv) {
	if (argc != 2) {
		std::cerr << "usage: tkw <wld-filename>.wld\n";
        return nullptr;
	}
	const char* filename = argv[1];
	if (!std::filesystem::exists(filename)) {
		std::cerr << filename << ": file not exists!\n";
        return nullptr;
	}
	if (
			std::filesystem::path(filename).extension()
			!= ".wld") {
		std::cerr << filename
			<< ": file type won't match (!= .wld)\n";
        return nullptr;
	}
    return filename;
}
