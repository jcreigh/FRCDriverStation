/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _RIOVERSIONS_H_
#define _RIOVERSIONS_H_

#include "narf/format.h"
#include "narf/tokenize.h"
#include <curl/curl.h>

std::string curlFirmwareVersion(uint16_t teamNum);
std::string curlLibVersion(uint16_t teamNum);

#endif /* _RIOVERSIONS_H_ */
