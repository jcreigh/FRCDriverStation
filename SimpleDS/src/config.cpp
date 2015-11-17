/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "config.h"

Config::Config(std::string filename) : filename_(filename), loaded(false) {
	loadFile(filename);
}

void Config::loadFile(std::string filename) {
	narf::MemoryFile file;
	if (loaded = file.read(filename)) {
		load(file.str());
	}
}

void Config::saveFile() {
	if (loaded) {
		narf::MemoryFile file;
		file.setData(save());
		if (!file.write(filename_)) {
			printf("Failed writing to config\n");
		}
	}
}
