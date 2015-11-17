/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _ENUMS_H_
#define _ENUMS_H_

#include <string>

enum Alliance {RED, BLUE};
extern std::string allianceNames[2];

enum Mode {TELEOP, TEST, AUTON};
extern std::string modeNames[3];

#endif /* _ENUMS_H_ */
