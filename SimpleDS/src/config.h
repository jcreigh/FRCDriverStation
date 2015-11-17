/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "narf/ini.h"
#include "narf/file.h"

class Config : public narf::INI::File {
	private:
		std::string filename_;
	public:
		bool loaded;
		Config(std::string filename);
		void loadFile(std::string filename);
		void saveFile();
};

#endif /* _CONFIG_H_ */
