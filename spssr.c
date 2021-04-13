#include <string.h>
#include "spssr.h"
#include "eoopc.h"

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
				    //exitAndCloseFile("Failed to allocate memory for variables.", "");
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
				self->variables_list_head->rec_type = 0;
				self->variables_list_head->type = 0;
				self->variables_list_head->write = 0;
				self->variables_list_head->next = NULL;
				
			eCALLna(self, readHeader);
			
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
				if (self->savptr == NULL) {
					return 1;
				}
				
			return 0;
			
		}

	/**
	* Parse the file header information
	* @param eOBJ
	* @return int (0 = true)
	*/
		int spssr_t_readHeader(void* eOBJ)
		{
			eSELF(spssr_t);
			
			(!self->silent) ? printf(cCYAN "Reading file header:\n" cRESET) : 0 ;
			
			//reset file pointer location to start
				fseek(self->savptr, 0, SEEK_SET);
			
			//Record type code @4
				self->readText(self, self->header.rec_type, 4);
				
				(!self->silent && self->debug) ? printf(cYELLOW "Record type code: " cMAGENTA "%s\n" cRESET, self->header.rec_type) : 0 ;
				
				if (strcmp(self->header.rec_type, "$FL2") != 0){
					fprintf(stderr, cRED "File must begin with chars $FL2 for a valid SPSS .sav file.\n\n" cRESET);
					exit(EXIT_FAILURE);
				}
			
			//read SPSS Version text @64
				self->readText(self, self->header.prod_name, 60);
				(!self->silent && self->debug) ? printf(cYELLOW "Endianness: " cMAGENTA "%s\n" cRESET, self->header.prod_name) : 0 ;
			
			//layout code should be 2 or 3 @68
				self->readInt32(self, &self->header.layout_code);

				if(self->header.layout_code != 2 && self->header.layout_code != 3)
					self->big_endian = true;

				(!self->silent && self->debug) ? printf(cYELLOW "Endianness: " cMAGENTA "%s\n" cRESET, ((self->big_endian) ? "Big" : "Little")) : 0 ;
				
			//Data elements per case @72
				self->readInt32(self, &self->header.nominal_case_size);
				(!self->silent && self->debug) ? printf(cYELLOW "Data elements per case: " cMAGENTA "%d\n" cRESET, self->header.nominal_case_size) : 0 ;
				
			//compression @76
				self->readInt32(self, &self->header.compression);
				(!self->silent && self->debug) ? printf(cYELLOW "Compression: " cMAGENTA "%s\n" cRESET, ((self->header.compression) ? "Yes" : "No")) : 0 ;
				
			//weight index @80
				self->readInt32(self, &self->header.weight_index);
				(!self->silent && self->debug) ? printf(cYELLOW "Weight Index: " cMAGENTA "%d\n" cRESET, self->header.weight_index) : 0 ;
				
			//cases @84
				self->readInt32(self, &self->header.ncases);
				(!self->silent && self->debug) ? printf(cYELLOW "Number of Cases: " cMAGENTA "%d\n" cRESET, self->header.ncases) : 0 ;
			
			//compression bias @92
				self->readFlt64(self, &self->header.compression_bias);
				(!self->silent && self->debug) ? printf(cYELLOW "Compression Bias: " cMAGENTA "%f\n" cRESET, self->header.compression_bias) : 0 ;

			// creation date
				//readOver((size_t)9, "Creation Date:");
				//readOver((size_t)8, "Creation Time:");
				
				//@109
				
			// file label
				//readOver((size_t)64, "File Label:");
				
				//@173
				
			// padding
				//readOver((size_t)3, "Padding:");
				
				//@176
				
			//if(!silent){
			//	printOut("\t%s Cases found", intToStr32(numberOfCases), "cyan");
			//}
			
			return 0;
		}
		
		void spssr_t_readInt32(void* eOBJ, int32_t* buffer)
		{
			eSELF(spssr_t);
			size_t read = fread(buffer, 1, 4, self->savptr);
		
			if(read != 4){
				fprintf(stderr, cRED "Failed to read 4 bytes in spssr_t_readInt32()\n\n" cRESET);
				exit(EXIT_FAILURE);
			}
		
			self->cursor += 4;
		
			(!self->silent && self->debug) ? printf(cRESET "[%d / %lu] ", 4, self->cursor) : 0 ;
		
		}
		
		void spssr_t_readFlt64(void* eOBJ, flt64_t* buffer)
		{
			eSELF(spssr_t);
			size_t read = fread(buffer, 1, 8, self->savptr);
		
			if(read != 8){
				fprintf(stderr, cRED "Failed to read 8 bytes in spssr_t_readFlt64()\n\n" cRESET);
				exit(EXIT_FAILURE);
			}
		
			self->cursor += 8;
		
			(!self->silent && self->debug) ? printf(cRESET "[%d / %lu] ", 8, self->cursor) : 0 ;
		
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
	
			self->cursor += amt;
			
			(!self->silent && self->debug) ? printf(cRESET "[%lu / %lu] ", amt, self->cursor) : 0 ;
				
		}
