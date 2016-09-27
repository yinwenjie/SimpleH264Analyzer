#include "stdafx.h"
#include "Configuration.h"

#include <fstream>

#if TRACE_CONFIG_LOGOUT
std::ofstream g_traceFile;
#endif