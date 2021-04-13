#include <string.h>
#include <unistd.h>
#include "eoopc.h"
#include "spssr.h"

int main(int argc, char *argv[])
{

	char* filename = "sav.sav";
	char* outputPrefix = "out";
	bool silent = false;
	bool debug = false;
	bool longCsv = true;
	bool includeRowIndex = false;
	int lineLimit = 1000000;
	
	int opt;
	
	// If the first character of optstring is '-', then each nonoption argv-element is handled as if
	// it were the argument of an option with character code 1. (This is used by programs that were written to expect options and other argv-elements in any order and that care about the ordering of the two.)
		if(argc == 2){
			
			//check for version output or help
				while ((opt = getopt(argc, argv, "-vh")) != -1) {
					
					if(opt == 'v'){
						printf( cGREEN "savtocsv " cRESET "version " cYELLOW "1.0 " cRESET "2021-04-02\n");
						exit(0);
					}
					
					else if(opt == 'h'){
						
						printf(cGREEN "\n----------SAV To CSV Help----------\n");
						
						printf(cYELLOW "Usage:\n");
						printf(cMAGENTA "\tcommand [options] [arguments]\n");
						
						printf(cYELLOW "Options:\n");
						printf(cGREEN "\t-f\t\t" cRESET "Set the input .sav filename (eg file.sav). Filename limited to 99 chars\n");
						
						printf(cGREEN "\t-o\t\t" cRESET "Set the output csv prefix (appended by x.csv where x is filenumber determined by Line Limit)."
									  " Filename limited to 99 chars");
						printf(cYELLOW "[default: out]\n");
						
						printf(cGREEN "\t-l\t\t" cRESET "Set Line Limit per csv file.");
						printf(cYELLOW "[default: 1000000]\n");
						
						printf(cGREEN "\t-s\t\t" cRESET "Set silent mode for no output.\n");
						
						printf(cGREEN "\t-d\t\t" cRESET "Set debug mode for additional output. Will not output if Silent mode on.\n");
						
						printf(cGREEN "\t-F\t\t" cRESET "Set csv output format to flat instead of long.\n");
						
						printf(cGREEN "\t-R\t\t" cRESET "Set csv output to include row index.\n");
						
						printf(cGREEN "\t-v\t\t" cRESET "Output version. Must be sole option.\n");
						
						printf(cGREEN "\t-h\t\t" cRESET "Output help. Must be sole option.\n");
						
						printf("\n");
						
						exit(0);
					}
					
				}
			
		}
	
	//reset getopt index for next while loop
		optind = 1;
	
	//check for silent first
		while ((opt = getopt(argc, argv, "-sfoldFR")) != -1) {
			
			if(opt == 's'){
				silent = true;
			}
			
		}
	
	//reset getopt index for next while loop
		optind = 1;
	
	//ullo
		(!silent) ? printf(cGREEN "\n----------SAV To CSV----------\n\n" cRESET) : 0 ;
	
	//if it's not -v or -h then is the num of args correct?
		if(argc <= 2){
			fprintf(stderr, cRED "Missing required options.\nUsage: savtocsv [-v] | [-f] [file...] [-o] [file...] [-l] [int] [-sdFR]\n\n" cRESET);
			exit(EXIT_FAILURE);
		}
	
	//go through normal options
		while ((opt = getopt(argc, argv, "-f:o:l:svdFR")) != -1) {
			
			switch (opt) {
				
				//get file pointer
					case 'f':
						filename = optarg;
					break;
					
				//get output filename
					case 'o':
						outputPrefix = optarg;
					break;
					
				//silent switch for stdout
					case 's':
						silent = true;
					break;
					
				//debug switch for stdout
					case 'd':
						debug = true;
					break;
					
				//csv file format
					case 'F':
						longCsv = false;
					break;
					
				//output row index for each row
					case 'R':
						includeRowIndex = true;
					break;
					
				//how pany lines per csv?
					case 'l':
						
						lineLimit = atoi(optarg);
						if(lineLimit == 0){
							fprintf(stderr, cRED "-l argument must be number > 0" cRESET);
							exit(EXIT_FAILURE);
						}
					
					break;
					
				//option not in optstring
					case '?':
					default:
						fprintf(stderr, cRED "Option not in option list of -f -o -l" cRESET);
						exit(EXIT_FAILURE);
				
			}
			
		}
	
	//check sav file option
		if(filename == NULL){
			fprintf(stderr, cRED "Missing required option -f" cRESET);
			exit(EXIT_FAILURE);
		}
		
	//show options
		if(!silent){
			printf(cYELLOW "Input file set: %s\n" cRESET, filename);
			printf(cYELLOW "Output file prefix set: %s\n" cRESET, outputPrefix);
			printf(cYELLOW "CSV Line Length set to: %d\n" cRESET, lineLimit);
			printf(cYELLOW "CSV file format default: %s\n" cRESET, ((longCsv) ? "Long" : "Flat"));
			printf(cYELLOW "CSV include row index default: %s\n" cRESET, ((includeRowIndex) ? "Yes" : "No"));
		}
		
	//make new reader object, and execute csv functions
		struct spssr_t* spssr = eNEW(spssr_t);
		eCONSTRUCT(spssr_t, spssr,
			filename,
			outputPrefix,
			silent,
			debug,
			longCsv,
			includeRowIndex,
			lineLimit
		);

	return 0;
}
