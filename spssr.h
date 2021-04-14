#ifndef SAV2CSV_SPSSR_H
#define SAV2CSV_SPSSR_H

#include <stdbool.h>
#include <stdio.h>
#include "common.h"
	
	/**
	* @var Header structure declaration
	*/
		struct header {
		
			//Record type code, either ‘$FL2’ for system files with uncompressed data or data compressed with simple byte
			//code compression, or ‘$FL3’ for system files with ZLIB compressed data.
			//
			//This is truly a character field that uses the character encoding as other strings. Thus, in a file with an
			//ASCII-based character encoding this field contains 24 46 4c 32 or 24 46 4c 33, and in a file with an
			//EBCDIC-based encoding this field contains 5b c6 d3 f2. (No EBCDIC-based ZLIB-compressed	files have been
			//observed.)
				char rec_type[5];
			
			//Product identification string. This always begins with the characters ‘@(#) SPSS DATA FILE’. PSPP uses the
			// remaining characters to give its version and the operating system name; for example, ‘GNU pspp 0.1.4 -
			// sparc-sun-solaris2.5.2’. The string is truncated if it would be longer than 60 characters; otherwise it is
			// padded on the right with spaces.
			//The product name field allow readers to behave differently based on quirks in the way that particular
			// software writes system files. See Value Labels Records, for the detail of the quirk that the PSPP system
			// file reader tolerates in files written by ReadStat, which has https://github.com/WizardMac/ReadStat
			// in prod_name.
				char prod_name[61];
			
			//Normally set to 2, although a few system files have been spotted in the wild with a value of 3 here.
			//use this value to determine the file’s integer endianness
				int32_t layout_code;
			
			//Number of data elements per case. This is the number of variables, except that long string variables add
			// extra data elements (one for every 8 characters after the first 8). However, string variables do not
			// contribute to this value beyond the first 255 bytes. Further, some software always writes -1 or 0 in this
			// field. In general, it is unsafe for systems reading system files to rely upon this value.
				int32_t nominal_case_size;
			
			//Set to 0 if the data in the file is not compressed, 1 if the data is compressed with simple bytecode
			// compression, 2 if the data is ZLIB compressed. This field has value 2 if and only if rec_type is ‘$FL3’.
				int32_t compression;
			
			//If one of the variables in the data set is used as a weighting variable, set to the dictionary index of
			// that variable, plus 1 (see Dictionary Index). Otherwise, set to 0.
				int32_t weight_index;
			
			//Set to the number of cases in the file if it is known, or -1 otherwise.
			//In the general case it is not possible to determine the number of cases that will be output to a system file
			// at the time that the header is written. The way that this is dealt with is by writing the entire system
			// file, including the header, then seeking back to the beginning of the file and writing just the ncases
			// field. For files in which this is not valid, the seek operation fails. In this case, ncases remains -1.
				int32_t ncases;
			
			//Compression bias, ordinarily set to 100. Only integers between 1 - bias and 251 - bias can be compressed.
			//By assuming that its value is 100, use bias to determine the file’s floating-point format and
			// endianness (see System File Format). If the compression bias is not 100, PSPP cannot auto-detect the
			// floating-point format and assumes that it is IEEE 754 format with the same endianness as the system file’s
			// integers, which is correct for all known system files.
				flt64_t compression_bias;
			
			//Date of creation of the system file, in ‘dd mmm yy’ format, with the month as standard English abbreviations,
			// using an initial capital letter and following with lowercase. If the date is not available then this field
			// is arbitrarily set to ‘01 Jan 70’.
				char creation_date[10];
			
			//Time of creation of the system file, in ‘hh:mm:ss’ format and using 24-hour time. If the time is not
			// available then this field is arbitrarily set to ‘00:00:00’.
				char creation_time[9];
			
			//File label declared by the user, if any (see FILE LABEL in PSPP Users Guide). Padded on the right with spaces.
			//A product that identifies itself as VOXCO INTERVIEWER 4.3 uses CR-only line ends in this field, rather
			// than the more usual LF-only or CR LF line ends.
				char file_label[65];
			
			//Ignored padding bytes to make the structure a multiple of 32 bits in length. Set to zeros.
				char padding[4];
			
		};
	
	/**
	* @var Variable structure declaration
	*/
		struct Variable{
			
			//Variable type code. Set to 0 for a numeric variable. For a short string variable or the first part of a
			// long string variable, this is set to the width of the string. For the second and subsequent parts of a
			// long string variable, set to -1, and the remaining fields in the structure are ignored.
				int32_t type;
			
			//If this variable has a variable label, set to 1; otherwise, set to 0.
				int32_t has_var_label;
			
			//If the variable has no missing values, set to 0. If the variable has one, two, or three discrete missing
			// values, set to 1, 2, or 3, respectively. If the variable has a range for missing variables, set to -2;
			// if the variable has a range for missing variables plus a single discrete value, set to -3.
			//A long string variable always has the value 0 here. A separate record indicates missing values for long
			// string variables
				int32_t n_missing_values;
			
			//The print and write members of sysfile_variable are output formats coded into int32 types. The
			// least-significant byte of the int32 represents the number of decimal places, and the next two bytes in
			// order of increasing significance represent field width and format type, respectively. The
			// most-significant byte is not used and should be set to zero.
			//
			//Format types are defined as follows:
			//
			//Value	Meaning
			//0	Not used.
			//1	A
			//2	AHEX
			//3	COMMA
			//4	DOLLAR
			//5	F
			//6	IB
			//7	PIBHEX
			//8	P
			//9	PIB
			//10	PK
			//11	RB
			//12	RBHEX
			//13	Not used.
			//14	Not used.
			//15	Z
			//16	N
			//17	E
			//18	Not used.
			//19	Not used.
			//20	DATE
			//21	TIME
			//22	DATETIME
			//23	ADATE
			//24	JDATE
			//25	DTIME
			//26	WKDAY
			//27	MONTH
			//28	MOYR
			//29	QYR
			//30	WKYR
			//31	PCT
			//32	DOT
			//33	CCA
			//34	CCB
			//35	CCC
			//36	CCD
			//37	CCE
			//38	EDATE
			//39	SDATE
			//40	MTIME
			//41	YMDHMS
			//A few system files have been observed in the wild with invalid write fields, in particular with value 0.
			// Readers should probably treat invalid print or write fields as some default format.
			//Print / write format for this variable.
				int32_t write;
				int32_t print;
			
			//Variable name. The variable name must begin with a capital letter or the at-sign (‘@’). Subsequent
			// characters may also be digits, octothorpes (‘#’), dollar signs (‘$’), underscores (‘_’), or full stops
			// (‘.’). The variable name is padded on the right with spaces.
			//The ‘name’ fields should be unique within a system file. System files written by SPSS that contain very
			// long string variables with similar names sometimes contain duplicate names that are later eliminated by
			// resolving the very long string names
				char name[9];
			
			//This field is present only if has_var_label is set to 1. It is set to the length, in characters, of the
			// variable label. The documented maximum length varies from 120 to 255 based on SPSS version, but some
			// files have been seen with longer labels.
				int32_t label_len;
			
			//This field is present only if has_var_label is set to 1. It has length label_len, rounded up to the
			// nearest multiple of 32 bits. The first label_len characters are the variable’s variable label.
				char* label;
			
			//This field is present only if n_missing_values is nonzero. It has the same number of 8-byte elements as
			// the absolute value of n_missing_values. Each element is interpreted as a number for numeric variables
			// (with HIGHEST and LOWEST indicated as described in the chapter introduction). For string variables of
			// width less than 8 bytes, elements are right-padded with spaces; for string variables wider than 8 bytes,
			// only the first 8 bytes of each missing value are specified, with the remainder implicitly all spaces.
			//For discrete missing values, each element represents one missing value. When a range is present, the
			// first element denotes the minimum value in the range, and the second element denotes the maximum value
			// in the range. When a range plus a value are present, the third element denotes the additional discrete
			// missing value.
				flt64_t* missing_values;
			
			//measure stype
				int32_t measure;
			
			//how many cols?
				int32_t cols;
				
			//alignment?
				int32_t alignment;
			
			//pointer to next element in linked list
				struct Variable* next;
				
		};
	
	/**
	* @var spssr_t public declaration
	*/
		struct spssr_t {
			
			char* filename;
			char* outputPrefix;
			bool silent;
			bool debug;
			bool longCsv;
			bool includeRowIndex;
			int lineLimit;
			
			//main file pointers
				FILE* savptr;
				FILE* csvptr;
				
			//switch to see if big endian for byte reversal
				bool big_endian;
				
			//counter to track cursor in file pointer
				uint64_t cursor;
				
			//header structure
				struct header header;
			
			//head element of Variables linked list
				struct Variable* variables_list_head;
				int variable_count;
				
			//methods
				int (*openFile)(void* eOBJ);
				void (*readHeader)(void* eOBJ);
				void (*readMeta)(void* eOBJ);
				void (*readVariable)(void* eOBJ);
				void (*readValueLabels)(void* eOBJ);
				void (*dataToCsvLong)(void* eOBJ);
				void (*dataToCsvFlat)(void* eOBJ);
				bool (*dubIsInt)(flt64_t val);
				void (*readUint8)(void* eOBJ, uint8_t * buffer);
				void (*readInt8)(void* eOBJ, int8_t * buffer);
				void (*readInt32)(void* eOBJ, int32_t * buffer);
				void (*readFlt64)(void* eOBJ, flt64_t * buffer);
				void (*readText)(void* eOBJ, char* buffer, size_t amt);
			
			
		};
		
		typedef struct spssr_t spssr_t;
		
		void spssr_t_instantiate(
			void* eOBJ,
			char* filename,
			char* outputPrefix,
			bool silent,
			bool debug,
			bool longCsv,
			bool includeRowIndex,
			int lineLimit
		);
		int spssr_t_openFile(void* eOBJ);
		void spssr_t_readHeader(void* eOBJ);
		void spssr_t_readMeta(void* eOBJ);
		void spssr_t_readVariable(void* eOBJ);
		void spssr_t_readValueLabels(void* eOBJ);
		void spssr_t_dataToCsvLong(void* eOBJ);
		void spssr_t_dataToCsvFlat(void* eOBJ);
		bool spssr_t_dubIsInt(flt64_t val);
		void spssr_t_readUint8(void* eOBJ, uint8_t* buffer);
		void spssr_t_readInt8(void* eOBJ, int8_t* buffer);
		void spssr_t_readInt32(void* eOBJ, int32_t* buffer);
		void spssr_t_readFlt64(void* eOBJ, flt64_t* buffer);
		void spssr_t_readText(void* eOBJ, char* buffer, size_t amt);

#endif //SAV2CSV_SPSSR_H
