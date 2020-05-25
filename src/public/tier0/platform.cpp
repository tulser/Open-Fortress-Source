// LINUX/OSX COMPATIBILITY FILE FOR GAMEUI
// (in public for some reason)
#include "platform.h"

#include <cstdio>
#include <fstream>

bool CopyFile(const char* source, const char* destination, bool dontoverwrite)
{
	if (dontoverwrite)
	{
		std::ifstream existcheck(destination);
		if (existcheck.good()) return 0;

	}
	std::ifstream from(source, std::ios::binary);
	std::ofstream to(destination, std::ios::binary);

	to << from.rdbuf();

	return 1;
}

bool DeleteFile(const char* file)
{
	return !std::remove(file);
}

char *itoa(int val, char* buf, int base)
{
	int i = 64;
	
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
		val /= base;

	return &buf[i+1];
}
