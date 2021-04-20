#include <math.h>
#include "spssr.h"
#include "eoopc.h"
	
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

	#define SND(f, ...) (!self->silent && self->debug) ? printf(f, __VA_ARGS__) : 0
	
	#define MIN(a,b) (((a)<(b))?(a):(b))

	/**
	* Instantiate a spssr object
	* @param eOBJ Self
	* @param filename .sav filename
	* @param outputPrefix
	* @param silent
	* @param debug
	* @param longCsv
	* @param includeRowIndex
	* @param lineLimit
	* @return void
	*/
		void spssr_t_instantiate(
			void* eOBJ,
			char* filename,
			char* outputPrefix,
			char* outputDirectory,
			bool silent,
			bool debug,
			bool longCsv,
			bool includeRowIndex,
			int lineLimit
		){
			eSELF(spssr_t);
			
			eTRY{
			
				//program settings
					self->filename = (char*) malloc (100);
					eCHECK(self->filename == 0, eERR, "Failed to allocate memory for filename string.");
					strcpy(self->filename, filename);
					
					self->outputPrefix = (char*) malloc (100);
					eCHECK(self->outputPrefix == 0, eERR, "Failed to allocate memory for output prefix string");
					strcpy(self->outputPrefix, outputPrefix);
					
					self->outputDirectory = (char*) malloc (100);
					eCHECK(self->outputDirectory == 0, eERR, "Failed to allocate memory for output directory string");
					strcpy(self->outputDirectory, outputDirectory);
					
					self->silent = silent;
					self->debug = debug;
					self->longCsv = longCsv;
					self->includeRowIndex = includeRowIndex;
					self->lineLimit = lineLimit;
				
				//switch to see if big endian for byte reversal
					self->big_endian = false;
					
				//counter to track cursor in file pointer
					self->cursor = 0;
					
				//methods
					self->openFile = &spssr_t_openFile;
					self->readHeader = &spssr_t_readHeader;
					self->readMeta = &spssr_t_readMeta;
					self->readVariable = &spssr_t_readVariable;
					self->readValueLabels = &spssr_t_readValueLabels;
					self->dataToCsvLong = &spssr_t_dataToCsvLong;
					self->dataToCsvFlat = &spssr_t_dataToCsvFlat;
					self->dubIsInt = &spssr_t_dubIsInt;
					self->readUint8 = &spssr_t_readUint8;
					self->readInt8 = &spssr_t_readInt8;
					self->readInt32 = &spssr_t_readInt32;
					self->readFlt64 = &spssr_t_readFlt64;
					self->readText = &spssr_t_readText;
					
				//try to open file
					eCALLna(self, openFile);
					eCHECK(self->savptr == NULL, eERR, "Unable to open file (permission denied, try sudo): %s", self->filename);
					(!self->silent) ? printf(cCYAN "Opened .sav file: %s\n" cRESET, filename): 0 ;
				
				//head element of Variables linked list
					self->variables_list_head = NULL;
					self->variables_list_head = (struct Variable*)malloc(sizeof(struct Variable));
					eCHECK(self->variables_list_head == NULL, eERR, "Failed to allocate memory for variables.");
					
					self->variables_list_head->alignment = 0;
					self->variables_list_head->cols = 0;
					self->variables_list_head->has_var_label = 0;
					self->variables_list_head->label = NULL;
					self->variables_list_head->label_len = 0;
					self->variables_list_head->measure = 0;
					self->variables_list_head->missing_values = NULL;
					self->variables_list_head->n_missing_values = 0;
					self->variables_list_head->print = 0;
					self->variables_list_head->type = 0;
					self->variables_list_head->write = 0;
					self->variables_list_head->next = NULL;
					
					self->variable_count = 0;
					
				//read file
					eCALLna(self, readHeader);
					eCALLna(self, readMeta);
				
					if(self->longCsv){
						eCALLna(self, dataToCsvLong);
					} else {
						eCALLna(self, dataToCsvFlat);
					}
					
				return;
				
			} eCATCH(eERR){
				if(self->filename) { free(self->filename); }
				if(self->outputPrefix) { free(self->outputPrefix); }
				if(self->outputDirectory) { free(self->outputDirectory); }
				if(self->savptr) { fclose(self->savptr); }
				exit(EXIT_FAILURE);
			}
			
		}
	
	/**
	* Try to open the savfile
	* @param eOBJ
	* @return void
	*/
		void spssr_t_openFile(void* eOBJ)
		{
			eSELF(spssr_t);
			self->savptr = NULL;
			self->savptr = fopen(self->filename, "rb");
		}

	/**
	* Parse the file header information
	* @param eOBJ
	* @return int (0 = true)
	*/
		void spssr_t_readHeader(void* eOBJ)
		{
			eSELF(spssr_t);
			
			(!self->silent) ? printf(cCYAN "Reading file header...\n" cRESET) : 0;
			
			//reset file pointer location to start
				fseek(self->savptr, 0, SEEK_SET);
			
			//Record type code @4
				self->readText(self, self->header.rec_type, 4);
				SND(cYELLOW "Record type code: " cMAGENTA "%s\n" cRESET, self->header.rec_type);
				eCHECK(strcmp(self->header.rec_type, "$FL2") != 0, 1, "File must begin with chars $FL2 for a valid SPSS .sav file.");

			//read SPSS Version text @64
				self->readText(self, self->header.prod_name, 60);
				SND(cYELLOW "Endianness: " cMAGENTA "%s\n" cRESET, self->header.prod_name);
			
			//layout code should be 2 or 3 @68
				self->readInt32(self, &self->header.layout_code);

				if(self->header.layout_code != 2 && self->header.layout_code != 3) {
					self->big_endian = true;
				}

				SND(cYELLOW "Endianness: " cMAGENTA "%s\n" cRESET, ((self->big_endian) ? "Big" : "Little"));
				
			//Data elements per case @72
				self->readInt32(self, &self->header.nominal_case_size);
				SND(cYELLOW "Data elements per case: " cMAGENTA "%d\n" cRESET, self->header.nominal_case_size);
				
			//compression @76
				self->readInt32(self, &self->header.compression);
				SND(cYELLOW "Compression: " cMAGENTA "%s\n" cRESET, ((self->header.compression) ? "Yes" : "No"));
				
			//weight index @80
				self->readInt32(self, &self->header.weight_index);
				SND(cYELLOW "Weight Index: " cMAGENTA "%d\n" cRESET, self->header.weight_index);
				
			//cases @84
				self->readInt32(self, &self->header.ncases);
				SND(cYELLOW "Number of Cases: " cMAGENTA "%d\n" cRESET, self->header.ncases);
			
			//compression bias @92
				self->readFlt64(self, &self->header.compression_bias);
				SND(cYELLOW "Compression Bias: " cMAGENTA "%f\n" cRESET, self->header.compression_bias);

			//creation date @109
				self->readText(self, self->header.creation_date, 9);
				SND(cYELLOW "Creation Date: " cMAGENTA "%s\n" cRESET, self->header.creation_date);
				self->readText(self, self->header.creation_time, 8);
				SND(cYELLOW "Creation Time: " cMAGENTA "%s\n" cRESET, self->header.creation_time);
				
			//file label @173
				self->readText(self, self->header.file_label, 64);
				SND(cYELLOW "File Label: " cMAGENTA "%s\n" cRESET, self->header.file_label);
				
			//padding @176
				self->readText(self, self->header.padding, 3);
				SND(cYELLOW "Padding: " cMAGENTA "%s\n" cRESET, self->header.padding);
				
			(!self->silent) ? printf(cCYAN "Cases found: " cMAGENTA "%d\n" cRESET, self->header.ncases) : 0 ;
			
			return;
			
			eCATCH(1){
				exit(EXIT_FAILURE);
			}
			
		}
		
	/**
	* Read from the sav file until the start of the data blocks
	* @return void
	*/
		void spssr_t_readMeta(void* eOBJ)
		{
			eSELF(spssr_t);
			
			eTRY{
			
				(!self->silent) ? printf(cCYAN "Reading meta data...\n" cRESET) : 0;
	
				bool stop = false;
				while (!stop) {
	
					int32_t recordType; self->readInt32(self, &recordType);
					SND(cYELLOW "Record Type:" cMAGENTA " [%d] \n\t" cRESET, recordType);
	
					switch (recordType) {
	
						// Variable Record (2)
							case RECORD_TYPE_VARIABLE:
								self->readVariable(self);
							break;
	
						// Value and labels (3)
							case RECORD_TYPE_VALUE_LABELS:
								self->readValueLabels(self);
							break;
	
						// Read and parse document records (6)
							case RECORD_TYPE_DOCUMENTS:
	
								{
									// number of variables
										int numberOfLines; self->readInt32(self, &numberOfLines);
										SND(cYELLOW "Number of Docs Vars:" cMAGENTA " [%d] \n\t" cRESET, numberOfLines);
	
									// read the lines
										int i;
										for (i = 0; i < numberOfLines; i++) {
											char doc[81]; self->readText(self, doc, 80);
											SND(cYELLOW "Doc Content:" cMAGENTA " [%s] \n" cRESET, doc);
										}
	
								}
	
							break;
	
						// Read and parse additional records (7)
							case RECORD_TYPE_ADDITIONAL:
	
								{
	
									int32_t subtype; self->readInt32(self, &subtype);
									SND(cYELLOW "Subtype:" cMAGENTA " [%d] " cRESET, subtype);
	
									int32_t size; self->readInt32(self, &size);
									SND(cYELLOW "Size:" cMAGENTA " [%d] " cRESET, size);
									
									int32_t count; self->readInt32(self, &count);
									SND(cYELLOW "Count:" cMAGENTA " [%d] " cRESET, count);
	
									int32_t datalen = size * count;
									SND(cYELLOW "Datalength:" cMAGENTA " [%d] \n\t" cRESET, datalen);
	
									switch (subtype) {
	
										// SPSS Record Type 7 Subtype 3 - Source system characteristics
											case 3: {
												
												//major version number. In version x.y.z, this is x.
												int32_t version_major; self->readInt32(self, &version_major);
												SND(cYELLOW "V Major::" cMAGENTA " [%d] " cRESET, version_major);
												
												//minor version number. In version x.y.z, this is y.
												int32_t version_minor;self->readInt32(self, &version_minor);
												SND(cYELLOW "V Minor:" cMAGENTA " [%d] " cRESET, version_minor);
												
												//version revision number. In version x.y.z, this is z.
												int32_t version_revision;self->readInt32(self, &version_revision);
												SND(cYELLOW "V Rev:" cMAGENTA " [%d] " cRESET, version_revision);
												
												//Machine code. PSPP always set this field to value to -1, but other values may appear.
												int32_t machine_code;self->readInt32(self, &machine_code);
												SND(cYELLOW "Machine Code:" cMAGENTA " [%d] \n\t" cRESET, machine_code);
												
												//Floating point representation code. For IEEE 754 systems this is 1.
												// IBM 370 sets this to 2, and DEC VAX E to 3.
												int32_t floating_point_rep;self->readInt32(self, &floating_point_rep);
												SND(cYELLOW "Flt Point Rep:" cMAGENTA " [%d] " cRESET, floating_point_rep);
												
												//Compression code. Always set to 1, regardless of whether or how the file is compressed.
												int32_t compression_code;self->readInt32(self, &compression_code);
												SND(cYELLOW "Compression Code:" cMAGENTA " [%d] " cRESET, compression_code);
												
												//Machine endianness. 1 indicates big-endian, 2 indicates little-endian.
												int32_t endianness;self->readInt32(self, &endianness);
												SND(cYELLOW "Endianness:" cMAGENTA " [%d] " cRESET, endianness);
												
												//Character code. The following values have been actually observed in
												// system files:
												//
												//1 - EBCDIC.
												//
												//2 - 7-bit ASCII.
												//
												//1250 - The windows-1250 code page for Central European and Eastern
												// European languages.
												//
												//1252 - The windows-1252 code page for Western European languages.
												//
												//28591 - ISO 8859-1.
												//
												//65001 - UTF-8.
												//
												//The following additional values are known to be defined:
												//
												//3 - 8-bit “ASCII”.
												//
												//4 - DEC Kanji.
												//
												//Other Windows code page numbers are known to be generally valid.
												//Old versions of SPSS for Unix and Windows always wrote value 2 in this
												// field, regardless of the encoding in use. Newer versions also write the character encoding as a string
												int32_t character_code; self->readInt32(self, &character_code);
												SND(cYELLOW "Char Code:" cMAGENTA " [%d] \n" cRESET, character_code);
												
											} break;
	
										//SPSS Record Type 7 Subtype 4 - Source system floating pt constants
											case 4: {
												
												//The system missing value
												flt64_t sysmis; self->readFlt64(self, &sysmis);
												SND(cYELLOW "Sysmis Val:" cMAGENTA " [%f] \n\t" cRESET, sysmis);
												
												//The value used for HIGHEST in missing values
												flt64_t highest; self->readFlt64(self, &highest);
												SND(cYELLOW "Highest:" cMAGENTA " [%f] \n\t" cRESET, highest);
												
												//The value used for LOWEST in missing values
												flt64_t lowest; self->readFlt64(self, &lowest);
												SND(cYELLOW "Lowest:" cMAGENTA " [%f] \n" cRESET, lowest);
												
											} break;
	
										// SPSS Record Type 7 Subtype 5 - Variable sets
											case 5: {
												char varSets[datalen+1]; self->readText(self, varSets, (size_t)datalen);
												SND(cYELLOW "Variable Sets:" cMAGENTA " [%s] \n" cRESET, varSets);
											} break;
	
										// SPSS Record Type 7 Subtype 6 - Trends date information
											case 6: {
												char trends[datalen+1]; self->readText(self, trends, (size_t)datalen);
												SND(cYELLOW "Trends Date Info:" cMAGENTA " [%s] \n" cRESET, trends);
											} break;
	
										// SPSS Record Type 7 Subtype 7 - Multi response groups
										//Record subtype. Set to 7 for records that describe multiple response sets
										// understood by SPSS before version 14, or to 19 for records that describe
										// dichotomy sets that use the CATEGORYLABELS=COUNTEDVALUES feature added in
										// version 14.
										//Zero or more line feeds (byte 0x0a), followed by a series of multiple response
										// sets, each of which consists of the following:
										//
										// - The set’s name (an identifier that begins with ‘$’), in mixed upper and lower
										//      case.
										// - An equals sign (‘=’).
										// - ‘C’ for a multiple category set, ‘D’ for a multiple dichotomy set with
										//      CATEGORYLABELS=VARLABELS, or ‘E’ for a multiple dichotomy set with
										//      CATEGORYLABELS=COUNTEDVALUES.
										// - For a multiple dichotomy set with CATEGORYLABELS=COUNTEDVALUES, a space,
										//      followed by a number expressed as decimal digits, followed by a space. If
										//      LABELSOURCE=VARLABEL was specified on MRSETS, then the number is 11;
										//      otherwise it is 1.2
										// - For either kind of multiple dichotomy set, the counted value, as a positive
										//      integer count specified as decimal digits, followed by a space, followed by
										//      as many string bytes as specified in the count. If the set contains numeric
										//      variables, the string consists of the counted integer value expressed as
										//      decimal digits. If the set contains string variables, the string contains
										//      the counted string value. Either way, the string may be padded on the right
										//      with spaces (older versions of SPSS seem to always pad to a width of 8
										//      bytes; newer versions don’t).
										// - A space.
										// - The multiple response set’s label, using the same format as for the counted
										//      value for multiple dichotomy sets. A string of length 0 means that the set
										//      does not have a label. A string of length 0 is also written if
										//      LABELSOURCE=VARLABEL was specified.
										// - A space.
										// - The short names of the variables in the set, converted to lowercase, each
										//      separated from the previous by a single space.
										// - Even though a multiple response set must have at least two variables, some
										//      system files contain multiple response sets with no variables or one
										//      variable. The source and meaning of these multiple response sets is unknown.
										//      (Perhaps they arise from creating a multiple response set then deleting all
										//      the variables that it contains?)
										// - One line feed (byte 0x0a). Sometimes multiple, even hundreds, of line feeds
										//      are present.
										
										/*
										Example: Given appropriate variable definitions, consider the following MRSETS command:
	
										MRSETS /MCGROUP NAME=$a LABEL='my mcgroup' VARIABLES=a b c
										       /MDGROUP NAME=$b VARIABLES=g e f d VALUE=55
										       /MDGROUP NAME=$c LABEL='mdgroup #2' VARIABLES=h i j VALUE='Yes'
										       /MDGROUP NAME=$d LABEL='third mdgroup' CATEGORYLABELS=COUNTEDVALUES
										        VARIABLES=k l m VALUE=34
										       /MDGROUP NAME=$e CATEGORYLABELS=COUNTEDVALUES LABELSOURCE=VARLABEL
										        VARIABLES=n o p VALUE='choice'.
										The above would generate the following multiple response set record of subtype 7:
										
										$a=C 10 my mcgroup a b c
										$b=D2 55 0  g e f d
										$c=D3 Yes 10 mdgroup #2 h i j
										It would also generate the following multiple response set record with subtype 19:
										
										$d=E 1 2 34 13 third mdgroup k l m
										$e=E 11 6 choice 0  n o p
										
										*/
											case 7: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Multi Response Groups:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
											
										//SPSS Record Type 7 Subtype 10 - Extra Product Info Record
										//A text string. A product that identifies itself as VOXCO INTERVIEWER 4.3 uses
										// CR-only line ends in this field, rather than the more usual LF-only or
										// CR LF line ends.
											case 10: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Extra Product Info:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// SPSS Record Type 7 Subtype 11 - Variable meta SPSS bits...
										//The remaining members are repeated count times, in the same order as the variable
										// records. No element corresponds to variable records that continue long string
										// variables.
											case 11:
		
												eCHECK(size != 4, eERR, "Error reading record type 7 subtype 11: bad "
									                "data element length [%d]. Expecting 4.", size);
									                
									            eCHECK((count % 3) != 0, eERR, "Error reading record type 7 subtype 11: "
									                "number of data elements [%d] is not a multiple of 3.", count);
	
												//go through vars and set meta
													struct Variable * current = self->variables_list_head;
													current = current->next;
	
													int i;
													for(i = 0; i < count/3; ++i){
														
														SND(cYELLOW "\tVar Meta for:" cMAGENTA " [%s] " cRESET, current->name);
														
														//The measurement type of the variable:
														//
														//1 - Nominal Scale
														//2 - Ordinal Scale
														//3 - Continuous Scale
														//
														//SPSS sometimes writes a measure of 0. PSPP interprets this as nominal scale.
														self->readInt32(self, &current->measure);
														SND(cYELLOW "Measure:" cMAGENTA " [%d] " cRESET, current->measure);
														
														//The width of the display column for the variable in characters.
														//This field is present if count is 3 times the number of variables
														// in the dictionary. It is omitted if count is 2 times the number
														// of variables.
														self->readInt32(self, &current->cols);
														SND(cYELLOW "Columns:" cMAGENTA " [%d] " cRESET, current->cols);
														
														//The alignment of the variable for display purposes:
														//
														//0 - Left aligned
														//1 - Right aligned
														//2 - Centre aligned
														self->readInt32(self, &current->alignment);
														SND(cYELLOW "Alignment:" cMAGENTA " [%d] \n" cRESET, current->alignment);
	
														current = current->next;
	
													}
	
											break;
	
										// SPSS Record Type 7 Subtype 13 - Extended names
										//A list of key–value tuples, where key is the name of a variable, and value is its
										// long variable name. The key field is at most 8 bytes long and must match the name
										// of a variable which appears in the variable record (see Variable Record). The
										// value field is at most 64 bytes long. The key and value fields are separated by
										// a ‘=’ byte. Each tuple is separated by a byte whose value is 09. There is no
										// trailing separator following the last tuple. The total length is count bytes.
											case 13: {
												char extendedNames[datalen+1]; self->readText(self, extendedNames, (size_t)datalen);
												SND(cYELLOW "Extended Names:" cMAGENTA " [%s] \n" cRESET, extendedNames);
											} break;
	
										// SPSS Record Type 7 Subtype 14 - Extended strings
										//Old versions of SPSS limited string variables to a width of 255 bytes. For
										// backward compatibility with these older versions, the system file format
										// represents a string longer than 255 bytes, called a very long string, as a
										// collection of strings no longer than 255 bytes each. The strings concatenated
										// to make a very long string are called its segments; for consistency, variables
										// other than very long strings are considered to have a single segment.
										//
										//A very long string with a width of w has n = (w + 251) / 252 segments, that is,
										// one segment for every 252 bytes of width, rounding up. It would be logical,
										// then, for each of the segments except the last to have a width of 252 and the
										// last segment to have the remainder, but this is not the case. In fact, each
										// segment except the last has a width of 255 bytes. The last segment has width
										// w - (n - 1) * 252; some versions of SPSS make it slightly wider, but not wide
										// enough to make the last segment require another 8 bytes of data.
										//
										//Data is packed tightly into segments of a very long string, 255 bytes per segment.
										// Because 255 bytes of segment data are allocated for every 252 bytes of the very
										// long string’s width (approximately), some unused space is left over at the end
										// of the allocated segments. Data in unused space is ignored.
										//
										//Example: Consider a very long string of width 20,000. Such a very long string has
										// 20,000 / 252 = 80 (rounding up) segments. The first 79 segments have width 255;
										// the last segment has width 20,000 - 79 * 252 = 92 or slightly wider (up to 96
										// bytes, the next multiple of 8). The very long string’s data is actually stored
										// in the 19,890 bytes in the first 78 segments, plus the first 110 bytes of the
										// 79th segment (19,890 + 110 = 20,000). The remaining 145 bytes of the 79th
										// segment and all 92 bytes of the 80th segment are unused.
										//
										//The very long string record explains how to stitch together segments to obtain
										// very long string data. For each of the very long string variables in the
										// dictionary, it specifies the name of its first segment’s variable and the very
										// long string variable’s actual width. The remaining segments immediately follow
										// the named variable in the system file’s dictionary.
										
										//A list of key–value tuples, where key is the name of a variable, and value is its
										// length. The key field is at most 8 bytes long and must match the name of a
										// variable which appears in the variable record (see Variable Record). The value
										// field is exactly 5 bytes long. It is a zero-padded, ASCII-encoded string that
										// is the length of the variable. The key and value fields are separated by a ‘=’
										// byte. Tuples are delimited by a two-byte sequence {00, 09}. After the last tuple,
										// there may be a single byte 00, or {00, 09}. The total length is count bytes.
											case 14: {
												char extendedStrings[datalen+1]; self->readText(self, extendedStrings, (size_t)datalen);
												SND(cYELLOW "Extended Strings:" cMAGENTA " [%s] \n" cRESET, extendedStrings);
											} break;
	
										// SPSS Record Type 7 Subtype 16 - Number Of Cases
											case 16: {
	
												int32_t byteOrder; self->readInt32(self, &byteOrder);
												SND(cYELLOW "Measure:" cMAGENTA " [%d] " cRESET, byteOrder);
												
												char skip[5]; self->readText(self, skip, 4);
												SND(cYELLOW "Skip:" cMAGENTA " [%s] " cRESET, skip);
												
												int32_t countHere; self->readInt32(self, &countHere);
												SND(cYELLOW "Count:" cMAGENTA " [%d] " cRESET, countHere);
												
												self->readText(self, skip, 4);
												SND(cYELLOW "Skip:" cMAGENTA " [%s] \n" cRESET, skip);
	
											} break;
	
										// SPSS Record Type 7 Subtype 17 - Dataset Attributes
											case 17: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Dataset Attributes:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// SPSS Record Type 7 Subtype 18 - Variable Attributes
											case 18: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Var Attributes:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// SPSS Record Type 7 Subtype 19 -  Extended multiple response groups
											case 19: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Extended multiple response groups:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// SPSS Record Type 7 Subtype 20 -  Encoding, aka code page
										//This record is not present in files generated by older software. See also the
										// character_code field in the machine integer info record (see character-code).
										//
										//When the character encoding record and the machine integer info record are both
										// present, all system files observed in practice indicate the same character
										// encoding, e.g. 1252 as character_code and windows-1252 as encoding, 65001 and
										// UTF-8, etc.
										//
										//If, for testing purposes, a file is crafted with different character_code and
										// encoding, it seems that character_code controls the encoding for all strings
										// in the system file before the dictionary termination record, including strings
										// in data (e.g. string missing values), and encoding controls the encoding for
										// strings following the dictionary termination record.
										
										//The name of the character encoding. Normally this will be an official IANA
										// character set name or alias. See http://www.iana.org/assignments/character-sets.
										// Character set names are not case-sensitive, but SPSS appears to write them in
										// all-uppercase.
											case 20: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Encoding, aka code page:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// SPSS Record Type 7 Subtype 21 - Extended value labels
											case 21: {
												
												int32_t var_name_len; self->readInt32(self, &var_name_len);
												SND(cYELLOW "Var Name Length:" cMAGENTA " [%d] " cRESET, var_name_len);
												
												char var_name[var_name_len+1];self->readText(self, var_name, (size_t)var_name_len);
												SND(cYELLOW "Var Name:" cMAGENTA " [%s] \n" cRESET, var_name);
												
												int32_t var_width; self->readInt32(self, &var_width);
												SND(cYELLOW "Var Width:" cMAGENTA " [%d] " cRESET, var_width);
												
												int32_t n_labels; self->readInt32(self, &n_labels);
												SND(cYELLOW "No. Labels:" cMAGENTA " [%d] \n\t" cRESET, n_labels);
												
												for(int j = 0; j < n_labels; j++){
												
													int32_t value_len; self->readInt32(self, &value_len);
													SND(cYELLOW "Val Length:" cMAGENTA " [%d] " cRESET, value_len);
													
													char value[value_len+1]; self->readText(self, value, (size_t)value_len);
													SND(cYELLOW "Value:" cMAGENTA " [%s] " cRESET, value);
													
													int32_t label_len; self->readInt32(self, &label_len);
													SND(cYELLOW "Label Length:" cMAGENTA " [%d] " cRESET, label_len);
													
													char  label[label_len+1]; self->readText(self, label, (size_t)label_len);
													SND(cYELLOW "Label:" cMAGENTA " [%s] \n" cRESET, label);
													
												}
												
											} break;
	
										// SPSS Record Type 7 Subtype 22 - Missing values for long strings
											case 22: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Missing values for long strings:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// SPSS Record Type 7 Subtype 23 - Sort Index information
											case 23: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "Sort Index information:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// SPSS Record Type 7 Subtype 24 - XML info
											case 24: {
												char attr[datalen+1]; self->readText(self, attr, (size_t)datalen);
												SND(cYELLOW "SXML info:" cMAGENTA " [%s] \n" cRESET, attr);
											} break;
	
										// Other info
											default: {
												char misc[datalen+1]; self->readText(self, misc, (size_t)datalen);
												SND(cYELLOW "Misc info:" cMAGENTA " [%s] \n" cRESET, misc);
											} break;
									}
	
								}
	
							break;
	
						// Finish
							case RECORD_TYPE_FINAL:
	
								//stop the loop
									stop = true;
	
								//ensure this value is 0
									int test;
									self->readInt32(self, &test);
									SND(cYELLOW "Test for final rec type: " cMAGENTA "%d\n" cRESET, test);
		
									if (test != 0) {
										eTHROW(eERR, "Error reading record type 999: Non-zero value found.");
									}
	
							break;
	
						default:
							eTHROW(eERR, "Read error: invalid record type [%d]", recordType);
	
					}
	
				}
				
				(!self->silent) ? printf(cCYAN "Variables found: " cMAGENTA "%d\n" cRESET, self->variable_count) : 0 ;
				
				return;
			
			} eCATCH(eERR){
				exit(EXIT_FAILURE);
			}
			
		}
		
	/**
	 * SPSS Record Type 2 - Variable information
	 * @throws \Exception
	 * @return void
	 */
		void spssr_t_readVariable(void* eOBJ)
		{
			eSELF(spssr_t);
			
			//addVariable(variablesList, typeCode, writeFormatCode);
				struct Variable * current = self->variables_list_head;
	
			//find last element
				while (current->next != NULL) {
					current = current->next;
				}
			
			//add new var
				current->next = (struct Variable *) malloc(sizeof(struct Variable));
			
			self->readInt32(self, &current->next->type);
			SND(cGREEN "((Var)) Type Code:" cMAGENTA " [%d] " cRESET, current->next->type);
			
			self->variable_count++;
			
			//if TYPECODE is -1, record is a continuation of a string var
				if(current->next->type == -1) {

					//read and ignore the next 24 bytes
						char tmp[25]; self->readText(self, tmp, 24);
						SND(cGREEN "String Continuation Var Skip 24:" cMAGENTA " [%s] " cRESET, tmp);

			//otherwise normal var. if numeric, type code here = 0. if string, type code is length of string.
				} else {

					// read label flag 0 or 1
						self->readInt32(self, &current->next->has_var_label);
						SND(cGREEN "Label?:" cMAGENTA " [%s] " cRESET, ((current->next->has_var_label == 1) ? "Yes" : "No"));

					// read missing value format code. Should be between -3 to 3
						self->readInt32(self, &current->next->n_missing_values);
						SND(cGREEN "Missing Value Code:" cMAGENTA " [%d] \n\t" cRESET, current->next->n_missing_values);
						eCHECK(abs(current->next->n_missing_values) > 3, 1, "Error reading variable Record: invalid "
								"missing value format code [%d]. Range is -3 to 3.", current->next->n_missing_values);

					// read print format code
						self->readInt32(self, &current->next->print);
						SND(cGREEN "Print Code:" cMAGENTA " [%d] " cRESET, current->next->print);

					// read write format code
						self->readInt32(self, &current->next->write);
						SND(cGREEN "Write Code:" cMAGENTA " [%d] \n\t" cRESET, current->next->write);

					// read short varname of 8 chars
						self->readText(self, current->next->name, 8);
						SND(cGREEN "Var Short Name:" cMAGENTA " [%s] " cRESET, current->next->name);

					// read label length and label only if a label exists
						if (current->next->has_var_label == 1) {

							self->readInt32(self, &current->next->label_len);
							SND(cGREEN "Label Length:" cMAGENTA " [%d] " cRESET, current->next->label_len);

							//need to ensure we read word-divisable amount of bytes
								int rem = 4-(current->next->label_len % 4);
								if(rem == 4){
									rem = 0;
								}
								
							current->next->label = (char*)malloc((size_t)((size_t)current->next->label_len+1));
							self->readText(self, current->next->label, (size_t)current->next->label_len);
							SND(cGREEN "Label:" cMAGENTA " [%s] " cRESET, current->next->label);
							
							if(rem > 0){
								char remtxt[rem+1];
								self->readText(self, remtxt, (size_t)rem);
							}
							SND(cGREEN "Skip:" cMAGENTA " [%d] " cRESET, rem);

						}

					// missing values
						if (current->next->n_missing_values != 0) {
							int i;
							for (i = 0; i < abs(current->next->n_missing_values); ++i) {
								flt64_t tmp;
								self->readFlt64(self, &tmp);
								SND(cGREEN "Missing Value:" cMAGENTA " [%f] " cRESET, tmp);
							}
						}
						
					current->next->measure = 0;
					current->next->cols = 0;
					current->next->alignment = 0;
					current->next->next = NULL;

				}
				
			(!self->silent && self->debug) ? printf(cRESET "\n\n") : 0 ;
			
			return;
			
			eCATCH(1){
				exit(EXIT_FAILURE);
			}
		}

	/**
	 * SPSS Record Type 3 - Value labels
	 * @return void
	 */
		void spssr_t_readValueLabels(void* eOBJ)
		{
			eSELF(spssr_t);
			
			eTRY{
			
				// number of labels
					int32_t numberOfLabels; self->readInt32(self, &numberOfLabels);
					SND(cBLUE "<<Var Labels>> Number of Labels:" cMAGENTA " [%d] \n\t" cRESET, numberOfLabels);
	
				// labels
					int i;
					for (i = 0; i < numberOfLabels; i++) {
	
						//(!self->silent) ? printf(cRESET "\n\t") : 0 ;
	
						// read the label value
							flt64_t labelValue; self->readFlt64(self, &labelValue);
							SND(cBLUE "Value:" cMAGENTA " [%f] " cRESET, labelValue);
							//@8
	
						// read the length of a value label
						// the following byte in an unsigned integer (max value is 60)
							int8_t labelLength; self->readInt8(self, &labelLength);
							SND(cBLUE "Label Length:" cMAGENTA " [%d] " cRESET, labelLength);
	
							//if (labelLength > 255) {
							//	exitAndCloseFile("The length of a value label(%s) must be less than 60.", doubleToStr(labelValue));
							//}
	
						//need to ensure we read word-divisable amount of bytes
							int rem = 8-((labelLength+1) % 8);
							if(rem == 8){
								rem = 0;
							}
	
							char label[labelLength+1];
							self->readText(self, label, (size_t)labelLength);
							SND(cBLUE "Label:" cMAGENTA " [%s] " cRESET, label);
							
							if(rem > 0){
								char remtxt[rem+1];
								self->readText(self, remtxt, (size_t)rem);
							}
							SND(cBLUE "Skip:" cMAGENTA " [%d] \n\t" cRESET, rem);
	
					}
	
				// read type 4 record (that must follow type 3!)
				// record type
					int32_t recTypeCode; self->readInt32(self, &recTypeCode);
					SND(cBLUE "Record Type Code:" cMAGENTA " [%d] " cRESET, recTypeCode);
					
					eCHECK(recTypeCode != RECORD_TYPE_VALUE_LABELS_INDEX, eERR,
						"Error reading Variable Index record: bad record type [%d]. Expecting Record Type 4.", recTypeCode);
	
				// number of variables to add to?
					int32_t numVars; self->readInt32(self, &numVars);
					SND(cBLUE "Vars to add:" cMAGENTA " [%d] Indexes: [" cRESET, numVars);
	
				// variableRecord indexes
					int j;
					for (j = 0; j < numVars; j++) {
	
						int32_t index; self->readInt32(self, &index);
						SND(cBLUE "%d " , index);
	
					}
					
				(!self->silent && self->debug) ? printf(cRESET "\n\n") : 0 ;
				
				return;
			
			} eCATCH(eERR){
				exit(EXIT_FAILURE);
			}
		}
		
	/**
	 * Convert data to long format csv's
	 * @return void
	 */
		void spssr_t_dataToCsvLong(void* eOBJ)
		{
			eSELF(spssr_t);
			
			eTRY{

				//initialise counters for the loops
					int fileNumber = 1;
					int caseid = 1;
					int rowCount = 1;
	
				//cluster for compression reads
					uint8_t cluster[8] = {0,0,0,0,0,0,0,0};
					int clusterIndex = 7;
	
				//how many rows overall?
					int totalRows = self->variable_count * self->header.ncases;
	
				//progress tracking
					int progressDivision;
					int progressCount = 0;
	
					if(totalRows < self->lineLimit){
						progressDivision = (int)ceil((flt64_t)totalRows / 20);
					} else {
						progressDivision = (int)ceil((flt64_t)self->lineLimit / 20);
					}
	
				//work out files and file pointers
					int filesAmount;
	
					if(totalRows > self->lineLimit){
						filesAmount = (totalRows / self->lineLimit) + 1;
					} else {
						filesAmount = 1;
					}
					FILE * csvs[filesAmount];
	
				//first filename
					char filename[201] = "";
					strcat(filename, self->outputDirectory);
					strcat(filename, self->outputPrefix);
					char filenameNum[100];
					int length = snprintf( NULL, 0, "%d", fileNumber );
					snprintf( filenameNum, (size_t)length+1, "%d", fileNumber );
					strcat(filename, filenameNum);
					strcat(filename, ".csv");
	
				//first file
					csvs[0] = fopen(filename, "w");
	
				//can we open and edit?
					eCHECK(csvs[0] == NULL, eERR, "Unable to open file (permission denied, try sudo): %s", filename);
	
				(!self->silent) ? printf(cCYAN "Building Long CSV:\n") : 0;
				(!self->silent) ? printf("\t%s\n" cRESET, filename) : 0;
	
				flt64_t numData;
	
				int i;
				for(i = 1; i <= self->header.ncases; i++){
	
					//loop through vars, skipping head of list
					struct Variable * current = self->variables_list_head;
	
					int variableId = 1;
					int j;
					for(j = 0; j < self->variable_count; j++){
					
						current = current->next;
	
						if(current->type != 0){
	
							//Same as "Width" column invariables tab in SPSS
							int charactersToRead = (current->write >> 8) & 0xFF; //byte 2
	
							int blocksToRead = (int)floor((((flt64_t)charactersToRead - 1) / 8) + 1);
	
							while (blocksToRead > 0) {
	
								if (self->header.compression > 0) {
	
									clusterIndex++;
									if(clusterIndex > 7){
	
										(!self->silent && self->debug) ? printf("\n\t") : 0;
										self->readUint8(self, &cluster[0]);
										SND(cBLUE " C1:" cMAGENTA " [%d] " cRESET, cluster[0]);
										self->readUint8(self, &cluster[1]);
										SND(cBLUE " C2:" cMAGENTA " [%d] " cRESET, cluster[1]);
										self->readUint8(self, &cluster[2]);
										SND(cBLUE " C3:" cMAGENTA " [%d] " cRESET, cluster[2]);
										self->readUint8(self, &cluster[3]);
										SND(cBLUE " C4:" cMAGENTA " [%d] " cRESET, cluster[3]);
										self->readUint8(self, &cluster[4]);
										SND(cBLUE " C5:" cMAGENTA " [%d] " cRESET, cluster[4]);
										self->readUint8(self, &cluster[5]);
										SND(cBLUE " C6:" cMAGENTA " [%d] " cRESET, cluster[5]);
										self->readUint8(self, &cluster[6]);
										SND(cBLUE " C7:" cMAGENTA " [%d] " cRESET, cluster[6]);
										self->readUint8(self, &cluster[7]);
										SND(cBLUE " C8:" cMAGENTA " [%d] \n" cRESET, cluster[7]);
	
										clusterIndex = 0;
	
									}
									
									//printf("\n-%d %d-\n", clusterIndex, cluster[clusterIndex]);
	
									switch (cluster[clusterIndex]) {
	
										// 0 skip this code (NULL code)
											case COMPRESS_SKIP_CODE:
										// 254 all blanks (8 spaces) - 254's will be inserted to make string up to width%8bytes in the compression cluster.
											case COMPRESS_ALL_BLANKS:
												charactersToRead -= 8;
											break;
	
										// 252 end of file, no more data to follow. This should not happen.
											case COMPRESS_END_OF_FILE:
												eTHROW(eERR, "Error reading data: unexpected end of compressed data file (cluster code 252)");
	
										// 253 data cannot be compressed, the value follows the cluster
											case COMPRESS_NOT_COMPRESSED:
	
												{
	
													// read a maximum of 8 characters but could be less if this is the last block
														size_t blockStringLength = (size_t)MIN(8, (float)charactersToRead);
	
													// append to existing value
														char txt[9];
														self->readText(self, txt, (size_t)blockStringLength);
														SND(cBLUE "%s\n" cRESET, txt);
	
													// if this is the last block, skip the remaining dummy byte(s) (in the block of 8 bytes)
														if (charactersToRead < 8) {
															char dummy[9];
															self->readText(self, dummy, (size_t)(8 - charactersToRead));
															SND(cBLUE "%s" cRESET, dummy);
														}
													// update the characters counter
														charactersToRead -= (int)blockStringLength;
	
												}
	
											break;
	
										// 255 system missing value
											case COMPRESS_MISSING_VALUE:
												eTHROW(eERR, "Error reading data: unexpected SYSMISS for string variable");
	
										// 1-251 value is code minus the compression BIAS (normally always equal to 100)
											default:
												eTHROW(eERR, "Error reading data: unexpected compression code for "
													"string variable %d", cluster[clusterIndex]);

									}
	
								//UNCOMPRESSED DATA
									} else {
	
										// read a maximum of 8 characters but could be less if this is the last block
											size_t blockStringLength = (size_t)MIN(8, (float)charactersToRead);
	
										// append to existing value
											char txt[9];
											self->readText(self, txt, (size_t)blockStringLength);
											SND(cBLUE "%s" cRESET, txt);
	
										// if this is the last block, skip the remaining dummy byte(s) (in the block of 8 bytes)
											if (charactersToRead < 8) {
												char dummy[9];
												self->readText(self, dummy, (size_t)(8 - charactersToRead));
												SND(cBLUE "%s" cRESET, dummy);
											}
										// update the characters counter
											charactersToRead -= (int)blockStringLength;
	
									}
	
								blocksToRead--;
								
								if(current->next != NULL && blocksToRead > 0){
									current = current->next;
									j++;
								}
	
							}
							
							continue;
	
						} else {
	
							bool insertNull = false;
							numData = 0;
	
							if(self->header.compression > 0){
	
								clusterIndex++;
								
								if(clusterIndex > 7){
	
									(!self->silent && self->debug) ? printf("\n\t") : 0;
									self->readUint8(self, &cluster[0]);
									SND(cBLUE " C1:" cMAGENTA " [%d] " cRESET, cluster[0]);
									self->readUint8(self, &cluster[1]);
									SND(cBLUE " C2:" cMAGENTA " [%d] " cRESET, cluster[1]);
									self->readUint8(self, &cluster[2]);
									SND(cBLUE " C3:" cMAGENTA " [%d] " cRESET, cluster[2]);
									self->readUint8(self, &cluster[3]);
									SND(cBLUE " C4:" cMAGENTA " [%d] " cRESET, cluster[3]);
									self->readUint8(self, &cluster[4]);
									SND(cBLUE " C5:" cMAGENTA " [%d] " cRESET, cluster[4]);
									self->readUint8(self, &cluster[5]);
									SND(cBLUE " C6:" cMAGENTA " [%d] " cRESET, cluster[5]);
									self->readUint8(self, &cluster[6]);
									SND(cBLUE " C7:" cMAGENTA " [%d] " cRESET, cluster[6]);
									self->readUint8(self, &cluster[7]);
									SND(cBLUE " C8:" cMAGENTA " [%d] \n" cRESET, cluster[7]);
	
									clusterIndex = 0;
	
								}
								
								//printf("\n-%d, %d-\n", clusterIndex, cluster[clusterIndex]);
								
								switch (cluster[clusterIndex]) {
	
									// skip this code
										case COMPRESS_SKIP_CODE:
										break;
	
									// end of file, no more data to follow. This should not happen.
										case COMPRESS_END_OF_FILE:
											eTHROW(eERR, "Error reading data: unexpected end of compressed "
								                    "data file (cluster code 252)");
	
									// data cannot be compressed, the value follows the cluster
										case COMPRESS_NOT_COMPRESSED:
											self->readFlt64(self, &numData);
										break;
	
									// all blanks
										case COMPRESS_ALL_BLANKS:
											numData = 0;
										break;
	
									// system missing value
										case COMPRESS_MISSING_VALUE:
											//used to be 'NULL' but LOAD DATA INFILE requires \N instead, otherwise a '0'
											// get's inserted instead
											insertNull = true;
										break;
	
									// 1-251 value is code minus the compression BIAS (normally always equal to 100)
										default:
											numData = cluster[clusterIndex] - self->header.compression_bias;
										break;
	
								}
	
							} else {
								self->readFlt64(self, &numData);
							}
	
							//write to file
	
								if(self->includeRowIndex){
									fprintf(csvs[fileNumber-1],"%d,",rowCount);
								}
	
								if(insertNull){
	
									fprintf(csvs[fileNumber-1],"%d,%d,\\N\n", caseid, variableId);
	
								} else if (self->dubIsInt(numData)) {
	
									fprintf(csvs[fileNumber-1],"%d,%d,%d\n", caseid, variableId, (int)numData);
	
								} else {
	
									fprintf(csvs[fileNumber-1],"%d,%d,%f\n", caseid, variableId, numData);
	
								}
	
							//switch to new file
							if(rowCount % self->lineLimit == 0){
	
								//close current file
									fclose(csvs[fileNumber-1]);
	
								//finish progressCount output
									if(progressCount < 20 && !self->silent){
										int x;
										for(x = 0; x <= (20 - progressCount); x++){
											printf("#");
										}
										fflush(stdout);
									}
	
									progressCount = 0;
	
								//make and open new file
									fileNumber++;
	
									char filenameHere[201] = "";
									strcat(filenameHere, self->outputDirectory);
									strcat(filenameHere, self->outputPrefix);
									char filenameNumHere[100];
									int lengthHere = snprintf( NULL, 0, "%d", fileNumber );
									snprintf( filenameNumHere, (size_t)lengthHere+1, "%d", fileNumber );
									strcat(filenameHere, filenameNumHere);
									strcat(filenameHere, ".csv");
	
									csvs[fileNumber-1] = fopen(filenameHere,"w");
	
									eCHECK(csvs[fileNumber-1] == NULL, eERR,
										"Unable to open file (permission denied, try sudo): %s", filenameHere);
	
								if(!self->silent){
									printf(cCYAN "\nBuilding Long CSV:");
									printf(cCYAN "\t%s" cRESET, filenameHere);
								}
	
							} else if (rowCount % progressDivision == 0){
	
								if(!self->silent){
									printf("#");
									fflush(stdout);
								}
	
								progressCount++;
	
							}
							
							variableId++;
							rowCount++;
	
						}
						
					}
	
					caseid++;
	
				}
	
				//finish progressCount output
					if(progressCount < 20 && !self->silent){
						int x;
						for(x = 0; x <= (20 - progressCount); x++){
							printf("#");
						}
						fflush(stdout);
					}
	
				if(!self->silent){
					printf(cGREEN "\n\nWrote %d rows. Wrote %d files.\n\n" cRESET, totalRows, filesAmount);
				}
	
				//close current file
					fclose(csvs[fileNumber-1]);
					
				return;
				
			} eCATCH(eERR){
				exit(EXIT_FAILURE);
			}
			
		}
		
	/**
	 * Convert data to flat format csv's
	 * @return void
	 */
		void spssr_t_dataToCsvFlat(void * eOBJ)
		{
			eSELF(spssr_t);
			
			eTRY{
		
				int fileNumber = 1;
				
				//cluster for compression reads
					uint8_t cluster[8] = {0,0,0,0,0,0,0,0};
					int clusterIndex = 7;
				
				//progress tracking
					int progressDivision;
					int progressCount = 0;
					
					if(self->header.ncases < self->lineLimit){
						progressDivision = (int)ceil((flt64_t)self->header.ncases / 20);
					} else {
						progressDivision = (int)ceil((flt64_t)self->lineLimit / 20);
					}
				
				//work out files and file pointers
					int filesAmount;
					
					if(self->header.ncases > self->lineLimit){
						filesAmount = (self->header.ncases / self->lineLimit) + 1;
					} else {
						filesAmount = 1;
					}
					
					FILE * csvs[filesAmount];
				
				//first filename
					char filename[201] = "";
					strcat(filename, self->outputDirectory);
					strcat(filename, self->outputPrefix);
					char filenameNum[100];
					int length = snprintf( NULL, 0, "%d", fileNumber );
					snprintf( filenameNum, (size_t)length+1, "%d", fileNumber );
					strcat(filename, filenameNum);
					strcat(filename, ".csv");
				
				//first file
					csvs[0] = fopen(filename, "w");
					
				//try to open
					eCHECK(csvs[0] == NULL, eERR, "Unable to open file (permission denied, try sudo): %s", filename);
				
				(!self->silent) ? printf(cCYAN "Building Flat CSV:\n") : 0;
				(!self->silent) ? printf("\t%s\n" cRESET, filename) : 0;
				
				flt64_t numData;
				
				int i;
				for(i = 1; i <= self->header.ncases; i++){
					
					//loop through vars, skipping head of list
					struct Variable * current = self->variables_list_head;
					
					if(self->includeRowIndex){
						fprintf(csvs[fileNumber-1],"%d,", i);
					}
					
					int j;
					int k = 0;
					for(j = 0; j < self->variable_count; j++, k++){
						
						current = current->next;
						
						if(current->type != 0){
						
							//Same as "Width" column invariables tab in SPSS
							int charactersToRead = (current->write >> 8) & 0xFF; //byte 2
							
							int blocksToRead = (int)floor((((flt64_t)charactersToRead - 1) / 8) + 1);
							
							while (blocksToRead > 0) {
								
								if (self->header.compression > 0) {
								
									clusterIndex++;
									if(clusterIndex > 7){
							
										(!self->silent && self->debug) ? printf("\n\t") : 0;
										self->readUint8(self, &cluster[0]);
										SND(cBLUE " C1:" cMAGENTA " [%d] " cRESET, cluster[0]);
										self->readUint8(self, &cluster[1]);
										SND(cBLUE " C2:" cMAGENTA " [%d] " cRESET, cluster[1]);
										self->readUint8(self, &cluster[2]);
										SND(cBLUE " C3:" cMAGENTA " [%d] " cRESET, cluster[2]);
										self->readUint8(self, &cluster[3]);
										SND(cBLUE " C4:" cMAGENTA " [%d] " cRESET, cluster[3]);
										self->readUint8(self, &cluster[4]);
										SND(cBLUE " C5:" cMAGENTA " [%d] " cRESET, cluster[4]);
										self->readUint8(self, &cluster[5]);
										SND(cBLUE " C6:" cMAGENTA " [%d] " cRESET, cluster[5]);
										self->readUint8(self, &cluster[6]);
										SND(cBLUE " C7:" cMAGENTA " [%d] " cRESET, cluster[6]);
										self->readUint8(self, &cluster[7]);
										SND(cBLUE " C8:" cMAGENTA " [%d] \n" cRESET, cluster[7]);
										
										clusterIndex = 0;
									
									}
									
									switch (cluster[clusterIndex]) {
										
										// skip this code
											case COMPRESS_SKIP_CODE:
										// all blanks
											case COMPRESS_ALL_BLANKS:
												charactersToRead -= 8;
											break;
										
										// end of file, no more data to follow. This should not happen.
											case COMPRESS_END_OF_FILE:
												eTHROW(eERR, "Error reading data: unexpected end of compressed "
								                    "data file (cluster code 252)");
											
										// data cannot be compressed, the value follows the cluster
											case COMPRESS_NOT_COMPRESSED:
											
												{
													
													// read a maximum of 8 characters but could be less if this is the last block
														size_t blockStringLength = (size_t)MIN(8, (float)charactersToRead);
		
													// append to existing value
														char txt[9];
														self->readText(self, txt, (size_t)blockStringLength);
														SND(cBLUE "%s" cRESET, txt);
		
													// if this is the last block, skip the remaining dummy byte(s) (in the block of 8 bytes)
														if (charactersToRead < 8) {
															char dummy[9];
															self->readText(self, dummy, (size_t)(8 - charactersToRead));
															SND(cBLUE "%s" cRESET, dummy);
														}
													// update the characters counter
														charactersToRead -= (int)blockStringLength;
													
												}
												
											break;
										
										// system missing value
											case COMPRESS_MISSING_VALUE:
												eTHROW(eERR, "Error reading data: unexpected SYSMISS for string variable");
										
										// 1-251 value is code minus the compression BIAS (normally always equal to 100)
											default:
												eTHROW(eERR, "Error reading data: unexpected compression code for string variable");
	
									}
								
								//UNCOMPRESSED DATA
									} else {
									
										// read a maximum of 8 characters but could be less if this is the last block
											size_t blockStringLength = (size_t)MIN(8, (float)charactersToRead);
		
										// append to existing value
											char txt[9];
											self->readText(self, txt, (size_t)blockStringLength);
											SND(cBLUE "%s" cRESET, txt);
		
										// if this is the last block, skip the remaining dummy byte(s) (in the block of 8 bytes)
											if (charactersToRead < 8) {
												char dummy[9];
												self->readText(self, dummy, (size_t)(8 - charactersToRead));
												SND(cBLUE "%s" cRESET, dummy);
											}
										// update the characters counter
											charactersToRead -= (int)blockStringLength;
											
									}
								
								blocksToRead--;
								
								if(current->next != NULL && blocksToRead > 0){
									current = current->next;
									j++;
								}
							
							}
							
							continue;
						
						}
						
						numData = 0;
						bool insertNull = false;
						
						if(self->header.compression > 0){
						
							clusterIndex++;
							if(clusterIndex > 7){
							
								(!self->silent && self->debug) ? printf("\n\t") : 0;
								self->readUint8(self, &cluster[0]);
								SND(cBLUE " C1:" cMAGENTA " [%d] " cRESET, cluster[0]);
								self->readUint8(self, &cluster[1]);
								SND(cBLUE " C2:" cMAGENTA " [%d] " cRESET, cluster[1]);
								self->readUint8(self, &cluster[2]);
								SND(cBLUE " C3:" cMAGENTA " [%d] " cRESET, cluster[2]);
								self->readUint8(self, &cluster[3]);
								SND(cBLUE " C4:" cMAGENTA " [%d] " cRESET, cluster[3]);
								self->readUint8(self, &cluster[4]);
								SND(cBLUE " C5:" cMAGENTA " [%d] " cRESET, cluster[4]);
								self->readUint8(self, &cluster[5]);
								SND(cBLUE " C6:" cMAGENTA " [%d] " cRESET, cluster[5]);
								self->readUint8(self, &cluster[6]);
								SND(cBLUE " C7:" cMAGENTA " [%d] " cRESET, cluster[6]);
								self->readUint8(self, &cluster[7]);
								SND(cBLUE " C8:" cMAGENTA " [%d] \n" cRESET, cluster[7]);
								
								clusterIndex = 0;
							
							}
							
							//printf("\n-%d, %d-\n", clusterIndex, cluster[clusterIndex]);
							
							switch (cluster[clusterIndex]) {
								
								// skip this code
									case COMPRESS_SKIP_CODE:
									break;
									
								// end of file, no more data to follow. This should not happen.
									case COMPRESS_END_OF_FILE:
										eTHROW(eERR, "Error reading data: unexpected end of compressed data file (cluster code 252)");
									
								// data cannot be compressed, the value follows the cluster
									case COMPRESS_NOT_COMPRESSED:
										self->readFlt64(self, &numData);
									break;
									
								// all blanks
									case COMPRESS_ALL_BLANKS:
										numData = 0;
									break;
									
								// system missing value
									case COMPRESS_MISSING_VALUE:
										//used to be 'NULL' but LOAD DATA INFILE requires \N instead, otherwise a '0' get's inserted instead
										insertNull = true;
									break;
									
								// 1-251 value is code minus the compression BIAS (normally always equal to 100)
									default:
										numData = cluster[clusterIndex] - self->header.compression_bias;
									break;
								
							}
						
						} else {
							self->readFlt64(self, &numData);
						}
						
						//write to file
						
							if(insertNull){
								
								fprintf(csvs[fileNumber-1],"\\N");
							
							} else if (self->dubIsInt(numData)) {
								
								fprintf(csvs[fileNumber-1],"%d",(int)numData);
							
							} else {
								
								fprintf(csvs[fileNumber-1],"%f",numData);
							
							}
							
							if(j < self->variable_count-1){
								fprintf(csvs[fileNumber-1],",");
							}
					
					}
					
					//newline
						fprintf(csvs[fileNumber-1],"\n");
					
					//switch to new file
						if(i % self->lineLimit == 0){
							
							//close current file
								fclose(csvs[fileNumber-1]);
								
							//finish progressCount output
								if(progressCount < 20 && !self->silent){
									int x;
									for(x = 0; x <= (20 - progressCount); x++){
										printf("#");
									}
									fflush(stdout);
								}
								
								progressCount = 0;
							
							//make and open new file
								fileNumber++;
								
								char filenameHere[201] = "";
								strcat(filenameHere, self->outputDirectory);
								strcat(filenameHere, self->outputPrefix);
								char filenameNumHere[100];
								int lengthHere = snprintf( NULL, 0, "%d", fileNumber );
								snprintf( filenameNumHere, (size_t)lengthHere+1, "%d", fileNumber );
								strcat(filenameHere, filenameNumHere);
								strcat(filenameHere, ".csv");
							
								csvs[fileNumber-1] = fopen(filenameHere,"w");
								
								eCHECK(csvs[fileNumber-1] == NULL, eERR, "Unable to open file (permission denied, try sudo): %s",
									filenameHere);
							
							if(!self->silent){
								printf(cCYAN "\nBuilding Flat CSV:");
								printf(cCYAN "\t%s" cRESET, filenameHere);
							}
							
						} else if (i % progressDivision == 0){
						
							if(!self->silent){
								printf("#");
								fflush(stdout);
							}
							
							progressCount++;
						
						}
				
				}
				
				//finish progressCount output
					if(progressCount < 20 && !self->silent){
						int x;
						for(x = 0; x <= (20 - progressCount); x++){
							printf("#");
						}
						fflush(stdout);
					}
				
				if(!self->silent){
					printf(cGREEN "\n\nWrote %d rows. Wrote %d files.\n\n" cRESET, self->header.ncases, filesAmount);
				}
				
				//close current file
					fclose(csvs[fileNumber-1]);
					
				return;
				
			} eCATCH(eERR){
				exit(EXIT_FAILURE);
			}
			
		}
		
	/**
	* Check if double can safely be treated as int
	* @param double val The double
	* @return bool
	*/
		bool spssr_t_dubIsInt(flt64_t val){
			int truncated = (int)val;
			return (val == truncated);
		}
		
	/**
	* Read an unsigned 8 bit integer from file
	* @param eOBJ
	* @param int8_t* buffer
	* @return void
	*/
		void spssr_t_readUint8(void* eOBJ, uint8_t* buffer)
		{
			eSELF(spssr_t);
			size_t read = fread(buffer, 1, 1, self->savptr);
		
			if(read != 1){
				fprintf(stderr, cRED "Failed to read 1 byte in spssr_t_readUint8()\n\n" cRESET);
				exit(EXIT_FAILURE);
			}
		
			self->cursor += 1;
		
			SND(cRESET "[%d / %lu] ", 1, self->cursor);
		
		}
		
	/**
	* Read a signed 8 bit integer from file
	* @param eOBJ
	* @param int8_t* buffer
	* @return void
	*/
		void spssr_t_readInt8(void* eOBJ, int8_t* buffer)
		{
			eSELF(spssr_t);
			size_t read = fread(buffer, 1, 1, self->savptr);
		
			if(read != 1){
				fprintf(stderr, cRED "Failed to read 1 byte in spssr_t_readInt8()\n\n" cRESET);
				exit(EXIT_FAILURE);
			}
		
			self->cursor += 1;
		
			SND(cRESET "[%d / %lu] ", 1, self->cursor);
		
		}
		
	/**
	* Read a signed 32 bit integer from file
	* @param eOBJ
	* @param int32_t* buffer
	* @return void
	*/
		void spssr_t_readInt32(void* eOBJ, int32_t* buffer)
		{
			eSELF(spssr_t);
			size_t read = fread(buffer, 1, 4, self->savptr);
		
			if(read != 4){
				fprintf(stderr, cRED "Failed to read 4 bytes in spssr_t_readInt32()\n\n" cRESET);
				exit(EXIT_FAILURE);
			}
		
			self->cursor += 4;
		
			SND(cRESET "[%d / %lu] ", 4, self->cursor);
		
		}
		
	/**
	* Read a signed 64 bit float from file
	* @param eOBJ
	* @param flt64_t* buffer
	* @return void
	*/
		void spssr_t_readFlt64(void* eOBJ, flt64_t* buffer)
		{
			eSELF(spssr_t);
			size_t read = fread(buffer, 1, 8, self->savptr);
		
			if(read != 8){
				fprintf(stderr, cRED "Failed to read 8 bytes in spssr_t_readFlt64()\n\n" cRESET);
				exit(EXIT_FAILURE);
			}
		
			self->cursor += 8;
		
			SND(cRESET "[%d / %lu] ", 8, self->cursor);
		
		}
		
	/**
	* Read bytes into a string buffer
	* @param char* buffer
	* @return int
	*/
		void spssr_t_readText(void* eOBJ, char* buffer, size_t amt)
		{
			eSELF(spssr_t);
			
			size_t read = fread(buffer, 1, amt, self->savptr);
			
			if(read != amt){
				fprintf(stderr, cRED "Failed to read %lu bytes in spssr_t_readText()\n\n" cRESET, amt);
				exit(EXIT_FAILURE);
			}
			
			buffer[amt] = '\0';
	
			self->cursor += amt;
			
			SND(cRESET "[%lu / %lu] ", amt, self->cursor);
				
		}
