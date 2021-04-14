#include <string.h>
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
			bool silent,
			bool debug,
			bool longCsv,
			bool includeRowIndex,
			int lineLimit
		){
			eSELF(spssr_t);
			
			//program settings
				self->filename = (char*) malloc (100);
				strcpy(self->filename, filename);
				
				self->outputPrefix = (char*) malloc (100);
				strcpy(self->outputPrefix, outputPrefix);
				
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
				
				self->readInt8 = &spssr_t_readInt8;
				self->readInt32 = &spssr_t_readInt32;
				self->readFlt64 = &spssr_t_readFlt64;
				self->readText = &spssr_t_readText;
				
			//try to open file
				if(eCALLna(self, openFile) != 0){
					fprintf(stderr, cRED "Unable to open file (permission denied, try sudo): %s\n\n" cRESET, self->filename);
					exit(EXIT_FAILURE);
				}
				
				(!self->silent) ? printf(cCYAN "Opened .sav file: %s\n" cRESET, filename): 0 ;
			
			//head element of Variables linked list
				self->variables_list_head = NULL;
				self->variables_list_head = (struct Variable*)malloc(sizeof(struct Variable));
				
				if (self->variables_list_head == NULL) {
				    fprintf(stderr, cRED "Failed to allocate memory for variables.\n\n" cRESET);
					exit(EXIT_FAILURE);
				}
				
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
			
		}
	
	/**
	* Try to open the savfile
	* @param eOBJ
	* @return int (0 = true)
	*/
		int spssr_t_openFile(void* eOBJ)
		{
			eSELF(spssr_t);
			
			//try to open for read in binary mode
				self->savptr = NULL;
				self->savptr = fopen(self->filename, "rb");
				
			//file open?
				if (self->savptr == NULL)
					return 1;
				
			return 0;
			
		}

	/**
	* Parse the file header information
	* @param eOBJ
	* @return int (0 = true)
	*/
		void spssr_t_readHeader(void* eOBJ)
		{
			eSELF(spssr_t);
			
			(!self->silent) ? printf(cCYAN "Reading file header:\n" cRESET) : 0;
			
			//reset file pointer location to start
				fseek(self->savptr, 0, SEEK_SET);
			
			//Record type code @4
				self->readText(self, self->header.rec_type, 4);
				
				SND(cYELLOW "Record type code: " cMAGENTA "%s\n" cRESET, self->header.rec_type);
				
				if (strcmp(self->header.rec_type, "$FL2") != 0){
					fprintf(stderr, cRED "File must begin with chars $FL2 for a valid SPSS .sav file.\n\n" cRESET);
					exit(EXIT_FAILURE);
				}
			
			//read SPSS Version text @64
				self->readText(self, self->header.prod_name, 60);
				SND(cYELLOW "Endianness: " cMAGENTA "%s\n" cRESET, self->header.prod_name);
			
			//layout code should be 2 or 3 @68
				self->readInt32(self, &self->header.layout_code);

				if(self->header.layout_code != 2 && self->header.layout_code != 3)
					self->big_endian = true;

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
			
		}
		
	/**
	* Read from the sav file until the start of the data blocks
	* @return void
	*/
		void spssr_t_readMeta(void* eOBJ)
		{
			eSELF(spssr_t);
			
			(!self->silent) ? printf(cCYAN "Reading meta data:\n" cRESET) : 0;

			bool stop = false;
			while (!stop) {

				int recordType;
				self->readInt32(self, &recordType);
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
			//
			//		// Read and parse document records (6)
			//			case RECORD_TYPE_DOCUMENTS:
			//
			//				{
			//					// number of variables
			//						int numberOfLines = readInt32("Number of Docs Vars:");
			//
			//					// read the lines
			//						int i;
			//						for (i = 0; i < numberOfLines; i++) {
			//							readOver((size_t)80, "Doc Content:");
			//						}
			//
			//				}
			//
			//			break;
			//
					// Read and parse additional records (7)
						case RECORD_TYPE_ADDITIONAL:

							{

								int32_t subtype;
								self->readInt32(self, &subtype);
								
								
								
								// = readInt32("SubType:");
								//@4

								//int size = readInt32("Size:");
								//@8

								//int count = readInt32("Count:");
								//@12

								//int datalen = size * count;
			//
			//					switch (subtype) {
			//
			//						// SPSS Record Type 7 Subtype 3 - Source system characteristics
			//							case 3:
			//								readOver((size_t)32, "Source system characteristics:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 4 - Source system floating pt constants
			//							case 4:
			//								readOver((size_t)24, "Source system floating pt constants:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 5 - Variable sets
			//							case 5:
			//								readOver((size_t)datalen, "Variable Sets:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 6 - Trends date information
			//							case 6:
			//								readOver((size_t)datalen, "Trends Date Info:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 7 - Multi response groups
			//							case 7:
			//								readOver((size_t)datalen, "Multi Response Groups:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 11 - Variable meta SPSS bits...
			//							case 11:
			//
			//								if (size != 4) {
			//									exitAndCloseFile("Error reading record type 7 subtype 11: bad data element length [%s]. Expecting 4.", intToStr32(size));
			//								}
			//
			//								if ((count % 3) != 0) {
			//									exitAndCloseFile("Error reading record type 7 subtype 11: number of data elements [%s] is not a multiple of 3.", intToStr32(size));
			//
			//								}
			//
			//								//go through vars and set meta
			//									struct Variable * current = variablesList;
			//									current = current->next;
			//
			//									int i;
			//									for(i = 0; i < count/3; ++i){
			//
			//										if(debug){
			//											printOut("~~~Var Meta~~~", "", "magenta");
			//											printOut("\n~~~Var Type: %s \n", intToStr32(current->type), "yellow");
			//										}
			//										current->measure =  readInt32("~~~Var Measure:");
			//										current->cols =  readInt32("~~~Var Cols:");
			//										current->alignment =  readInt32("~~~Var Alignment:");
			//
			//										current = current->next;
			//
			//									}
			//
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 13 - Extended names
			//							case 13:
			//								readOver((size_t)datalen, "Extended Names:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 14 - Extended strings
			//							case 14:
			//								readOver((size_t)datalen, "Extended Strings:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 16 - Number Of Cases
			//							case 16:
			//
			//								readInt32("Byte Order:");
			//								readOver(4, "Skip:");
			//								readInt32("Count:");
			//								readOver(4, "Skip:");
			//
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 17 - Dataset Attributes
			//							case 17:
			//								readOver((size_t)datalen, "Dataset Attributes:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 18 - Variable Attributes
			//							case 18:
			//								readOver((size_t)datalen, "Variable Attributes:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 19 -  Extended multiple response groups
			//							case 19:
			//								readOver((size_t)datalen, "Extended multiple response groups:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 20 -  Encoding, aka code page
			//							case 20:
			//								readOver((size_t)datalen, "Encoding, aka code page:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 21 - Extended value labels
			//							case 21:
			//								readOver((size_t)datalen, "Extended value labels:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 22 - Missing values for long strings
			//
			//							case 22:
			//								readOver((size_t)datalen, "Missing values for long strings:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 23 - Sort Index information
			//							case 23:
			//								readOver((size_t)datalen, "Sort Index information:");
			//							break;
			//
			//						// SPSS Record Type 7 Subtype 24 - XML info
			//							case 24:
			//								readOver((size_t)datalen, "XML info:");
			//							break;
			//
			//						// Other info
			//							default:
			//								readOver((size_t)datalen, "Misc info:");
			//							break;
			//					}
			//
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
									fprintf(stderr, cRED "Error reading record type 999: Non-zero value found.\n\n" cRESET);
									exit(EXIT_FAILURE);
								}

						break;

					default:
						fprintf(stderr, cRED "Read error: invalid record type [%d]\n\n" cRESET, recordType);
						exit(EXIT_FAILURE);

				}

			}
			
			(!self->silent) ? printf(cCYAN "Variables found: " cMAGENTA "%d\n" cRESET, self->variable_count) : 0 ;
		
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
			SND(cYELLOW "((Variable)) " cGREEN "Type Code:" cMAGENTA " [%d] " cRESET, current->next->type);
			
			//if TYPECODE is -1, record is a continuation of a string var
				if(current->next->type == -1) {

					//read and ignore the next 24 bytes
						char tmp[25]; self->readText(self, tmp, 24);
						SND(cGREEN "String Continuation Var Skip 24:" cMAGENTA " [%s] " cRESET, tmp);

			//otherwise normal var. if numeric, type code here = 0. if string, type code is length of string.
				} else {

					self->variable_count++;

					// read label flag 0 or 1
						self->readInt32(self, &current->next->has_var_label);
						SND(cGREEN "Label?:" cMAGENTA " [%s] " cRESET, ((current->next->has_var_label == 1) ? "Yes" : "No"));

					// read missing value format code. Should be between -3 to 3
						self->readInt32(self, &current->next->n_missing_values);
						SND(cGREEN "Missing Value Code:" cMAGENTA " [%d] \n\t" cRESET, current->next->n_missing_values);

						if (abs(current->next->n_missing_values) > 3) {
							fprintf(stderr, cRED "Error reading variable Record: invalid missing value format code [%d]. Range is -3 to 3.\n" cRESET, current->next->n_missing_values);
							exit(EXIT_FAILURE);
						}

					// read print format code
						self->readInt32(self, &current->next->print);
						SND(cGREEN "Print Code:" cMAGENTA " [%d] " cRESET, current->next->print);

					// read write format code
						self->readInt32(self, &current->next->write);
						SND(cGREEN "Write Code:" cMAGENTA " [%d] \n\t" cRESET, current->next->write);

					// read short varname of 8 chars
						self->readText(self, current->next->name, 8);
						SND(cGREEN "Var Short Name:" cRESET " [%s] " , current->next->name);

					// read label length and label only if a label exists
						if (current->next->has_var_label == 1) {

							self->readInt32(self, &current->next->label_len);
							SND(cGREEN "Label Length:" cRESET " [%d] ", current->next->label_len);

							//need to ensure we read word-divisable amount of bytes
								int rem = 4-(current->next->label_len % 4);
								if(rem == 4){
									rem = 0;
								}
								
							current->next->label = (char*)malloc((size_t)((size_t)current->next->label_len+1));
							self->readText(self, current->next->label, (size_t)current->next->label_len);
							SND(cGREEN "Label:" cRESET " [%s] ", current->next->label);
							
							if(rem > 0){
								char remtxt[rem+1];
								self->readText(self, remtxt, (size_t)rem);
							}
							SND(cGREEN "Skip:" cRESET " [%d] ", rem);

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
				
			(!self->silent) ? printf(cRESET "\n\n") : 0 ;
			
		}

	/**
	 * SPSS Record Type 3 - Value labels
	 * @return void
	 */
		void spssr_t_readValueLabels(void* eOBJ)
		{
			eSELF(spssr_t);
			
			// number of labels
				int32_t numberOfLabels; self->readInt32(self, &numberOfLabels);
				SND(cYELLOW "<<Var Labels>> " cGREEN "Number of Labels:" cMAGENTA " [%d] " cRESET, numberOfLabels);

			// labels
				int i;
				for (i = 0; i < numberOfLabels; i++) {

					(!self->silent) ? printf(cRESET "\n\t") : 0 ;

					// read the label value
						flt64_t labelValue; self->readFlt64(self, &labelValue);
						SND(cGREEN "Value:" cMAGENTA " [%f] " cRESET, labelValue);
						//@8

					// read the length of a value label
					// the following byte in an unsigned integer (max value is 60)
						int8_t labelLength; self->readInt8(self, &labelLength);
						SND(cGREEN "Label Length:" cMAGENTA " [%d] " cRESET, labelLength);

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
						SND(cGREEN "Label:" cMAGENTA " [%s] " cRESET, label);
						
						if(rem > 0){
							char remtxt[rem+1];
							self->readText(self, remtxt, (size_t)rem);
						}
						SND(cGREEN "Skip:" cMAGENTA " [%d] " cRESET, rem);

				}

				(!self->silent) ? printf(cRESET "\n\t") : 0 ;

			// read type 4 record (that must follow type 3!)
			// record type
				int32_t recTypeCode; self->readInt32(self, &recTypeCode);
				SND(cGREEN "Record Type Code:" cMAGENTA " [%d] " cRESET, recTypeCode);
				
				if (recTypeCode != 4) {
					fprintf(stderr, cRED "Error reading Variable Index record: bad record type [%d]. Expecting Record Type 4.\n" cRESET, recTypeCode);
					exit(EXIT_FAILURE);
				}

			// number of variables to add to?
				int32_t numVars; self->readInt32(self, &numVars);
				SND(cGREEN "Vars to add:" cMAGENTA " [%d] Indexes: [" cRESET, numVars);

			// variableRecord indexes
				int j;
				for (j = 0; j < numVars; j++) {

					int32_t index; self->readInt32(self, &index);
					SND(cGREEN "%d " , index);

				}
				
			(!self->silent) ? printf(cMAGENTA "]\n" cRESET) : 0 ;
		
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
