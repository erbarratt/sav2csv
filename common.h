#ifndef SAV2CSV_COMMON_H
#define SAV2CSV_COMMON_H

#include <stdint.h>
	
	typedef double flt64_t;
	
	#define  cRED "\x1b[31m"
	#define  cGREEN "\x1b[32m"
	#define  cYELLOW "\x1b[33m"
	#define  cBLUE "\x1b[34m"
	#define  cMAGENTA "\x1b[35m"
	#define  cCYAN "\x1b[36m"
	#define  cRESET "\x1b[0m"
	
	#define RECORD_TYPE_VARIABLE 2
	#define RECORD_TYPE_VALUE_LABELS 3
	#define RECORD_TYPE_VALUE_LABELS_INDEX 4
	#define RECORD_TYPE_DOCUMENTS 6
	#define RECORD_TYPE_ADDITIONAL 7
	#define RECORD_TYPE_FINAL 999
	
	#define COMPRESS_SKIP_CODE 0
	#define COMPRESS_END_OF_FILE 252
	#define COMPRESS_NOT_COMPRESSED 253
	#define COMPRESS_ALL_BLANKS 254
	#define COMPRESS_MISSING_VALUE 255
	
#endif //SAV2CSV_COMMON_H
