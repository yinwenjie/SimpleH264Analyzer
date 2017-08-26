#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <fstream>

#define TRACE_CONFIG_CONSOLE 1
#define TRACE_CONFIG_LOGOUT 1
#define TRACE_CONFIG_TEMP_LOG 1

#define TRACE_CONFIG_SEQ_PARAM_SET 1
#define TRACE_CONFIG_PIC_PARAM_SET 1

#define TRACE_CONFIG_SLICE_HEADER 1

#define TRACE_CONFIG_MACROBLOCK 1

#define TRACE_CONFIG_MACROBLOCK_RESIDUAL 1

// Trace file declaration..
extern std::ofstream g_traceFile;
extern std::ofstream g_tempFile;

#endif
