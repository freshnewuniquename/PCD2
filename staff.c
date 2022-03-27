#include<ctype.h>	// toupper()
#include<stdbool.h>	// bool, true, false
#include<stdio.h>	// fread(), fseek(), ftell(), fwrite(), getchar(), perror(), printf(), rewind(), scanf(), ungetc(), EOF, FILE, SEEK_END, stdin
#include<stdlib.h>	// atoi(), free(), malloc()
#include<string.h>	// memset(), strcmp(), strcpy(), strlen()
#include<time.h>	// localtime(), time(), time_t, struct tm


typedef unsigned long long u64;


/*
	This enum list all the modifiable fields found in the Staff{}.
	This enum will be used in staffPromptDetails() to determine which field is going to be modified.

	XXX: Maximum 127 enums if it's 1 byte. Sign bit is used for other purposes.
*/
enum StaffModifiableFields { SE_ID, SE_NAME, SE_POSITION, SE_PHONE };
#define STAFF_ENUM_LENGTH 4


// Define maximum char array size needed for Staff struct elements buffer.
#define STAFF_BUF_MAX 128

// Define how many rows of records to display in displayStaff().
#define ENTRIES_PER_PAGE 16

// Define a small function to truncate remaining bytes in stdin.
#define truncate() 													\
	do {															\
		int _tmpChar;												\
		while((_tmpChar = getchar()) != '\n' && _tmpChar != EOF);	\
	} while(0)

// Define a small function for user to see the output before proceeding.
// A portable solution instead of the Windows only system("pause").
#define pause()									\
	do {										\
		printf("Enter any key to proceed. ");	\
		truncate();								\
		putchar('\n');							\
	} while(0)

// Define whether or not to use clear screen function.
#define ENABLE_CLS false

#define HEADER ""

// Define a small function to clear the screen.
#define cls()						\
	do {							\
		if(ENABLE_CLS) {			\
			system("clear || cls");	\
		} else {					\
			putchar('\n');			\
			putchar('\n');			\
			putchar('\n');			\
		}							\
		printf(HEADER);				\
	} while(0)



/*
	Data dictionary
	===============
	Name			Example				Description
	----			------				-----------
	ID				S0001				An unique ID assigned to the staff.
	name			John Smith			Stores the name of the staff.
	position		Admin				A string that records the position of the staff.
	phone			0123456789			A string that stores the phone number of the staff. Defaults to MYS country code of +60
	passHash		<64 bits binary>	A read-only hash of the staff's password.

	XXX:	passHash will be set to 0 if the staff was deleted.
			Upon setting to 0, the first 32 bits will be used for date logging.
			32 bits|8 bits|8 bits|16 bits (Big endian layout as an example)
			zeroes  day    month  year

	NOTE:	Modify $STAFF_BUF_MAX as well if the largest buffer size is changed (current max, $name, is 128 bytes).
*/
// TODO: maybe a salt that is produced with the answer of a security question and also maybe make a reset password thing.
typedef struct {
	char id[6];
	struct {
		char name[128];
		char position[32];
		char phone[16];
	} details;
	// 2 bytes padding.
	u64 passHash;
} Staff;


// Instead of initialising this with the normal struct initialisation.
// Get a copy of this from StaffDisplayOptionsInit(), filled with default values.
// Then only modify the fields' value.
typedef struct {
	char* header;							// Message to pass in to retain printed contents if isInteractive is true.
	char** idList;	 						// A char array that contains a 6 bytes string staff ID to include/exclude.
	bool displayList[STAFF_ENUM_LENGTH];	// An bool array to determine which staff field to print.
	int idListLen;							// Length of $idList.
	int entriesPerPage;						// The number of entries to display per page.
	int page;								// The currently opened page.
	bool isInclude;							// Determine to only print the ID passed in or exclude them and print non-matching.
	bool isInteractive;						// Whether to prompt the user for page navigation or not.
	bool displayDeleted;					// Print deleted staff details or ignore it. 
	bool displayExisting;					// Print deleted staff details or ignore it.
	struct {								// A struct that contains the metadata of the current search. (Non-modifiable)
		int totalBytes;						// Total bytes of the staff file.
		int totalEntries;					// Total number of entries in the staff file.
		int matchedLength;					// Total matched entries.
	} metadata;
	// No padding required yay.
} StaffDisplayOptions;


// ----- START OF HEADERS -----
/*
	Error codes:
		> 0 	No error.
		-1/EOF	EOF signal received.
		-2		User aborted the operation.
		-3		File/stdio error.
		-4		Heap allocation/stdlib error
		-15		Miscellaneous errors.
*/

/**
 * @brief	Presents a screen to add more staff to the staff record.
 * 
 * @retval	0	Staff successfully added.
 * @retval	EOF	Staff addition cancelled (EOF signal received).
 * @retval	-2	Staff addition cancelled (Add operation was cancelled by user).
 * @retval	-3	Staff failed to be added (File operation error).
 */
int staffAdd(void);


/**
 * @brief	Presents a screen to add more staff to the staff record.
 * 
 * @retval	0	Staff successfully added.
 * @retval	EOF	Staff search cancelled (EOF signal received).
 * @retval	-2	Staff search cancelled (Search operation was cancelled by user).
 * @retval	-3	Staff search failed (File operation error).
 */
int staffSearch(void);


/**
 * @brief	Select a staff to modify their details.
 *
 * @retval	0	Staff successfully modified.
 * @retval	EOF	Staff modification cancelled (EOF signal received).
 * @retval	-2	Staff modification cancelled (Modify operation was cancelled by user).
 * @retval	-3	Staff failed to be modified (File operation error).
 */
int staffModify(void);


/**
 * @brief	Presents a screen with all the member details in a table format.
 *
 * @retval	0	Staffs details successfully displayed.
 * @return		Negative value indicating error. (See staffDisplaySelected() error codes)
 */
int staffDisplay(void);


/**
 * @brief	Present a screen that show past employee with a brief summary.
 *
 * @retval	0	Staffs details successfully displayed and quit normally.
 * @retval	EOF	Staff report opertion aborted (EOF received).
 * @retval	-3	Staff failed to be displayed (File operation error).
 */
int staffReport(void);

/**
 * @brief	Select staffs to delete.
 *
 * @retval	EOF	Staff deletion failed (EOF signal received).
 * @retval	-15	Staff deletion failed (An error occured).
 * @return		Number of staffs deleted.
 */
int staffDelete(void);


/**
 * @brief	Prompts the user to enter selected staff details and validate them.
 *
 * This function handles the user input by validating them for correctness.
 * The inputted data will be truncated to the appropriate size to avoid buffer overflow.
 * This function makes the assumption that $buf is appropriately sized (>= to the size of corresponding field in Staff{}).
 * Any data truncated will not be notified and is up to the callee to show the user what was left of their input.
 * stdin buffer will always be read to after the newline character so next scanf() read will not be required to rewind() first.
 * If EOF is received during the prompt, $buf is left in an incomplete state. (May contain invalid data)
 *
 * @param	buf				A pointer to the appropriately sized buffer.
 * @param	selection		An enum containing the field that is going to be validated. Bit flip of enum will disable prompt message.
 * @param	specialStrings	A bool indicating whether to validate the data or not.
 *
 * @retval	0	Read and validated successfully.
 * @retval	EOF	EOF signal received, abort prompt.
 * @retval	-2	Read but not validated (Exited with :q or :w).
 * @retval	-15	Read but not validated (Not implemented).
 */
int staffPromptDetails(char* buf, enum StaffModifiableFields selection);


/**
 * @brief	Returns a copy of a StaffDisplayOptions{} filled with the default values.
 *
 * All modifiable fields are:					\n
 * Parameter				Default				\n
 * char* header;			(NULL)				\n
 * char** idList;			(NULL)				\n
 * bool displayList[$N];	(all set to false)	\n
 * int idListLen;			(0)					\n
 * int entriesPerPage;		($E)				\n
 * int page;				(0)					\n
 * bool isInclude;			(true)				\n
 * bool isInteractive;		(true)				\n
 * bool displayDeleted;		(false)				\n
 * bool displayExisting;	(true)				\n
 *
 * $N = $STAFF_ENUM_LENGTH, $E = $ENTRIES_PER_PAGE
 *
 * @return	A struct with the default values filled.
 */
StaffDisplayOptions staffDisplayOptionsInit(void);


/**
 * @brief	Display the passed staff information in a nice table format.
 *
 * @param	displayOptions	A pointer to the display options for the staff data.
 *
 * @retval	EOF	Print operation aborted (EOF signal received).
 * @retval	-3	Print operation failed (File operation error).
 * @retval	-4	Print operation failed (Allcoation operation error).
 * @return		Number of fields matched.
 */
int staffDisplaySelected(StaffDisplayOptions* options);


/**
 * @brief	Presents the user with a login screen and prompts the user to login.
 *
 * A login screen that allows staffs to login.
 * The staff's ID will be used to determine which staff is planning to login.
 * If the staff's ID is valid and matched one of the records, a password will be prompted.
 * If the inputted password is incorrect thrice, staff ID will be prompted again in case the staff ID was entered incorrectly.
 * A complete Staff struct with the corresponding staff details will be returned, after the staff has logged in.
 *
 * @return	First byte of ID is filled with 0 to indicate error, EOF for termination with EOF.
 * @return	Logged in user's Staff struct.
 */
Staff staffLogin(void);


/**
 * @brief	Hashes the inputted string (password) to a one-way BLAKE2 hash.
 *
 * A BLAKE2 message digest hash function that takes in a message to produce a fixed sized hash.
 * This implementation of the BLAKE2 hash is in accordance with the BLAKE2b algorithm, designed for 64 bits system.
 * This implementation does not include keys.
 * Salting isn't in the scope of this hashing function.
 *
 * Message inputted to this function is also limited to only (2**64)-1 bytes due to the reduced size of length variable.
 * Any bytes after the null-terminated string will be ignored and will not affect the result of the hash.
 *
 * References:
 * 	RFC 7693: https://datatracker.ietf.org/doc/html/rfc7693
 *
 * @param 	msg	A pointer to a char array to be hashed.
 * @return		8 bytes hash of the string (password).
 */
u64 computeHash(char* msg);


/**
 * @brief	Compress function defined by BLAKE2b.
 *
 * @param	hash		Pointer to the 64 bytes hash array.
 * @param	msg			Pointer to one of the 128 bytes chunk of the inputted message.
 * @param	compressed	Number of bytes that has been compressed (including $msg).
 * @param	isLastBlock	A boolean value that tells whether or not the current message chunk is the last one.
 */
void BLAKE2bF(u64* hash, char* msg, u64 compressed, bool isLastBlock);


/**
 * @brief	Mixing function defined by BLAKE2b.
 *
 * @param	v	Pointer to $v array that contains hashes and IV constants.
 * @param	a	Defined index to take and mix.
 * @param	b	Defined index to take and mix.
 * @param	c	Defined index to take and mix.
 * @param	d	Defined index to take and mix.
 * @param	x	One of 4 bytes character from the 128 bytes message chunk chosen with SIGMA.
 * @param	y	One of 4 bytes character from the 128 bytes message chunk chosen with SIGMA.
 */
void BLAKE2bG(u64* v, u64 a, u64 b, u64 c, u64 d, u64 x, u64 y);


/**
 * @brief	Searches the current stirng and return index of first occurence if there is a match.
 *
 * NOTE: This implementation is only limited to $STAFF_BUF_MAX-1 characters.
 *
 * @param	text	Text to be searched.
 * @param	query	Query to search the text provided.
 *
 * @retval	-1	No match.
 * @return		First index of the match.
 */
int KMPSearch(char* text, char* query);



int staffAdd(void) {
	int retval = 0;

	Staff newStaff;
	FILE* staffFile = fopen("staff.bin", "ab+");

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	// Password max length is $STAFF_BUF_MAX-1.
	// scanf() will scan an additional character to determine if password exceeds intended size.
	// So an additional character is allocated for password's null character.
	char buf[STAFF_BUF_MAX+1];
	enum StaffModifiableFields curPrompt = SE_ID;

	// Prompt ID
	// XXX: Iterate through enum by incrementing.
	// XXX: Added one to print the last field after prompting it.
	while(curPrompt < (STAFF_ENUM_LENGTH+1)) {
		cls();
		printf(
			"ADD STAFF\n"
			"=========\n"
		);

		// Print entered details after they have been filled.
		if(curPrompt > SE_ID) {
			printf("Staff ID       : %s\n", newStaff.id);
		}
		if(curPrompt > SE_NAME) {
			printf("Staff name     : %s\n", newStaff.details.name);
		}
		if(curPrompt > SE_POSITION) {
			printf("Staff position : %s\n", newStaff.details.position);
		}
		if(curPrompt > SE_PHONE) {
			printf("Staff phone    : %s\n", newStaff.details.phone);
		}

		if(curPrompt > SE_ID) {
			// Insert a line break to separate from the table.
			putchar('\n');
		}
		printf("Enter :q to abort at any point.\n\n");

		int res;
		switch(curPrompt) {
			case SE_ID:
				res = staffPromptDetails(buf, SE_ID);
				if(res == EOF) {
					retval = EOF;
					goto CLEANUP;
				}
				if(res != 0) {
					break;
				}

				// Ensure the Staff ID entered is unique.
				Staff tmp;
				bool exists = false;

				while(fread(&tmp, sizeof(Staff), 1, staffFile) == 1) {
					if(strcmp(tmp.id, buf) == 0 && (tmp.passHash & 0xFFFFFFFF00000000) != 0) {
						exists = true;
						break;
					}
				}
				rewind(staffFile);

				if(exists) {
					printf("Staff with the same ID exists!\n\n");
					pause();
					res = -15;
				} else {
					strcpy(newStaff.id, buf);
				}
				break;
			case SE_NAME:
				res = staffPromptDetails(buf, SE_NAME);
				if(res == 0) {
					strcpy(newStaff.details.name, buf);
				}
				break;
			case SE_POSITION:
				res = staffPromptDetails(buf, SE_POSITION);
				if(res == 0) {
					strcpy(newStaff.details.position, buf);
				}
				break;
			case SE_PHONE:
				res = staffPromptDetails(buf, SE_PHONE);
				if(res == 0) {
					strcpy(newStaff.details.phone, buf);
				}
				break;
			default:;
				// Do nothing.
		}

		if(res == 0) {
			++curPrompt;
		} else if(res == -2) {
			printf("Staff addition aborted!\n");
			pause();
			retval = -2;
			goto CLEANUP;
		} else if(res == EOF) {
			retval = EOF;
			goto CLEANUP;
		}
	}

	// Prompt password
	while(1) {
		// NOTE: Implemented independantly because password hash should not be modifiable.
		printf("Password (E.g. Ky54asdf' _.dQw4w9WgXcQ): ");
		
		// Scan an additional character to determine if user's password exceeds max length.
		if(scanf("%128[^\n]", buf) == EOF) {
			retval = EOF;
			goto CLEANUP;
		}
		u64 hash1 = computeHash(buf);
		int passLen = strlen(buf);
		memset(buf, 0, STAFF_BUF_MAX); // Zero out buffer to erase sensitive data.
		truncate();

		if(passLen == STAFF_BUF_MAX) {
			printf("Please enter a password that is shorter than 127 characters!\n");
			pause();
			continue;
		}

		printf("Re-enter password: ");
		// Don't have to scan an additional character anymore.
		if(scanf("%127[^\n]", buf) == EOF) {
			retval = EOF;
			goto CLEANUP;
		}
		u64 hash2 = computeHash(buf);
		memset(buf, 0, STAFF_BUF_MAX-1); // NULL is already zero, don't have to zero it again.
		truncate();

		if(hash1 == hash2) {
			newStaff.passHash = hash1;
			break;
		} else {
			printf("Passwords do not match!\n\n");
		}
	}

	printf("\nAre you sure you want to save the current staff detail? [Yn]: ");
	if(scanf("%c", buf) == EOF) {
		retval = EOF;
		goto CLEANUP;
	}
	truncate();

	if(toupper(*buf) == 'N') {
		printf("\nStaff addition aborted!\n");
		pause();
		retval = -2;
		goto CLEANUP;
	}

	if(fwrite(&newStaff, sizeof(newStaff), 1, staffFile) == 0) {
		printf("An error occured while writing to file buffer!\n");
		pause();
		retval = -3;
		goto CLEANUP;
	}

CLEANUP:
	if(fclose(staffFile) == EOF) {
		perror("Error (Closing staff file) ");
		pause();
		retval = -3;
	} else if(retval == 0) {
		printf("New staff details saved successfully!\n\n");
		pause();
	}

	return retval;
}


int staffSearch(void) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "rb");
	Staff* staffArr = NULL;
	char* matches = NULL;
	char** matchesPtr = NULL;

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		pause();
		retval = 1;
		goto CLEANUP;
	}

	// Read whole file to memory.
	if(fseek(staffFile, 0, SEEK_END) != 0) {
		perror("Error (fseek)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	int len = ftell(staffFile);
	rewind(staffFile);

	if(len == -1L) {
		perror("Error (ftell)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	staffArr = malloc(len);

	if(fread(staffArr, len, 1, staffFile) != 1) {
		perror("Error (fread)");
		pause();
		retval = -3;
		goto CLEANUP;
	}
	rewind(staffFile);

	// Set up array to keep matches.
	#define ID_SIZE 6

	StaffDisplayOptions s = staffDisplayOptionsInit();

	int curCapacity = 1024;
	matches = malloc(ID_SIZE*1024);
	matchesPtr = malloc(1024*sizeof(char*));

	if(matches == NULL) {
		perror("Error (malloc matches)");
		pause();
		retval = -4;
		goto CLEANUP;
	}
	if(matchesPtr == NULL) {
		perror("Error (malloc $matchesPtr)");
		pause();
		retval = -4;
		goto CLEANUP;
	}

	int* matchesLen = &s.idListLen;

	for(int i = 0; i < curCapacity; ++i) {
		matchesPtr[i] = matches+i*6;
	}

	s.idList = matchesPtr;
	s.displayList[SE_NAME] = true;
	s.displayList[SE_ID] = true;
	s.displayList[SE_POSITION] = true;
	s.displayList[SE_PHONE] = true;
	s.isInteractive = false;

	char buf[STAFF_BUF_MAX];

	// TODO: realloc matches when full.
	while(1) {
		cls();
		printf(
			"SEARCH STAFF\n"
			"============\n"
			"(Enter :q to quit.)\n"
			"(Enter :h for help.)\n"
		);
		staffDisplaySelected(&s);

		// TODO: add on to previous search.
		//printf("([+]Field=Query): ");
		printf("(Field=Query): ");
		int res;
		res = scanf("%127[^=\n]", buf);
		getchar(); // Consume newline or equal character.
		// truncate() not needed anymore.

		if(res == EOF) {
			retval = EOF;
			goto CLEANUP;
		}

		// toupper $buf.
		int i = 0;
		while(buf[i]) {
			buf[i] = toupper(buf[i]);
			++i;
		}

		enum StaffModifiableFields field = -1;

		if(buf[0] == ':') {
			if(buf[1] == 'Q' || buf[1] == 'W') {
				retval = -2;
				goto CLEANUP;
			} else if(buf[1] == 'H') {
				cls();
				printf(
					"Help:\n"
					"  Type in field name and search query to search.\n"
					// TODO: feature to add on to previous search.
					// "Precede the field name with a '+' sign to add on to the search result."
					"  The search query may contain special characters such as:\n"
					"    '%%' to match zero or more characters.\n"
					"    '_' to match any character.\n\n"
					"  Usage:\n"
					"    $FIELD=$QUERY\n"
					"  E.g.:\n"
					"    Name=J%%\n"
					"    (This searches for any name that starts with a capital 'J')\n"
				);
				pause();
				continue;
			}
		} else if(strcmp(buf, "ID") == 0) {
			field = SE_ID;
		} else if(strcmp(buf, "NAME") == 0) {
			field = SE_NAME;
		} else if(strcmp(buf, "POSITION") == 0) {
			field = SE_POSITION;
		} else if(strcmp(buf, "PHONE") == 0) {
			field = SE_PHONE;
		} else {
			printf("Entered field does not match any of the field!\n");
			pause();
			continue;
		}

		// Replace buffer again with search query.
		if(scanf("%127[^\n]", buf) == EOF) {
			retval = EOF;
			goto CLEANUP;
		}
		truncate();

		int wildCardlastIdx = -1;
		for(int i = 0; i < (int) strlen(buf); ++i) {
			if(buf[i] == '%') {
				wildCardlastIdx = i;
			}
		}

		// Will only enter here if a field is matched.
		for(int i = 0; i < (int) (len/sizeof(Staff)); ++i) {
			if((staffArr[i].passHash & 0xFFFFFFFF00000000) != 0) {
				char* text;

				switch(field) {
					case SE_ID:
						text = staffArr[i].id;
						break;
					case SE_NAME:
						text = staffArr[i].details.name;
						break;
					case SE_POSITION:
						text = staffArr[i].details.position;
						break;
					case SE_PHONE:
						text = staffArr[i].details.phone;
						break;
					default:
						retval = -15;
						break;
				}
				int textLen = strlen(text);
				int queryLen = strlen(buf);

				int textOffset = 0;
				int stringIdx = -1;
				bool match = true;
				// XXX: Temporarily replace null char with %
				buf[queryLen] = '%';
				for(int qIdx = 0; qIdx <= queryLen; ++qIdx) {
					if((buf[qIdx] == '_' || buf[qIdx] == '%') && stringIdx != -1) {
						printf("!=-1 offset: %d, sidx: %d, q: %d\n", textOffset, stringIdx, qIdx);
						// pause();
						// Extract string to search. (Was in extracting string mode)
						// Temporarily replace special character with null terminator.
						char c = buf[qIdx];
						buf[qIdx] = 0;

						int res;
						if(qIdx == wildCardlastIdx) {
							// Try to match the remaining length of the text to find a true match.
							// Only possible characters here all will only take up 1 character space.
							int textOffsetLocal = textOffset;
							while(textOffsetLocal < textLen) {
								res = KMPSearch(text+textOffsetLocal, buf+stringIdx);
								if(res == -1 || textLen-res-textOffsetLocal == queryLen-qIdx) {
									break;
								} else {
									textOffsetLocal += res+1;
								}
							}
							textOffset = textOffsetLocal;
						} else {
							res = KMPSearch(text+textOffset, buf+stringIdx);
						}
						buf[qIdx] = c;

						if(res == -1 || (res != 0 && (stringIdx == 0 || buf[stringIdx-1] == '_'))) {
							match = false;
							break;
						}
						textOffset += res+qIdx-stringIdx;
						stringIdx = -1;
					} else if(stringIdx == -1) {
						printf("==-1 offset: %d, sidx: %d, q: %d\n", textOffset, stringIdx, qIdx);
						// pause();
						// Normal iteration.
						if(buf[qIdx] == '_') {
							++textOffset;
						} else if(buf[qIdx] != '%') {
							stringIdx = qIdx;
						}
					}

					if(textOffset > textLen) {
						match = false;
						break;
					}
				}
				buf[queryLen] = 0;
				if(textOffset != textLen && buf[queryLen-1] != '%') {
					match = false;
				}

				if(match) {
					strcpy(matchesPtr[*matchesLen], staffArr[i].id);
					++*matchesLen;
				}
			}
		}
	}

CLEANUP:
	#undef ID_SIZE
	fclose(staffFile);
	free(matches);
	free(matchesPtr);
	return retval;
}


int staffModify(void) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "rb+");
	
	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		retval = -3;
		goto CLEANUP;
	}

	Staff chosenStaff;
	
	char buf[STAFF_BUF_MAX];
	char id[6];

	int numModified = 0;
	while(1) {
		cls();
		printf(
			"MODIFY STAFF\n"
			"============\n"
		);
	
		printf("Type a staff ID to modify their staff details or :q to quit.\n\n");
		int res = staffPromptDetails(id, SE_ID);
		if(res == EOF) {
			retval = EOF;
			goto CLEANUP;
		}

		if(res == -2) {
			printf("Staff modification aborted!\n");
			pause();
			retval = -2;
			goto CLEANUP;
		}

		// Search for matching records.
		bool found = false;
		while(fread(&chosenStaff, sizeof(Staff), 1, staffFile) != 0) {
			if(strcmp(chosenStaff.id, id) == 0 && (chosenStaff.passHash & 0xFFFFFFFF00000000) != 0) {
				found = true;
				break;
			}
		}

		int curFilePos = ftell(staffFile);
		if(curFilePos == -1) {
			perror("Error (ftell)");
			retval = -3;
			goto CLEANUP;
		}
		rewind(staffFile);
		
		if(found) {
			while(true) {
				cls();
				printf(
					"MODIFY STAFF\n"
					"============\n"
					"(Enter :q to quit modification.)\n"
					"(Enter :w to save modification.)\n"
					"(Enter :h for help)\n\n"
					"ID       : %s\n"
					"Name     : %s\n"
					"Position : %s\n"
					"Phone    : %s\n\n"
					"(Field=Value): ",
					chosenStaff.id, chosenStaff.details.name, chosenStaff.details.position, chosenStaff.details.phone
				);
				scanf("%127[^=\n]", buf);
				getchar(); // consume equal sign or newline.
				// truncate() not needed anymore if newline.

				// toupper() for string.
				int i = 0;
				while(buf[i]) {
					buf[i] = toupper(buf[i]);
					++i;
				}

				int res = 0;
				if(strcmp(buf, "ID") == 0) {
					res = staffPromptDetails(buf, ~SE_ID);
					if(res == 0) {
						bool exists = false;
						while(fread(&chosenStaff, sizeof(Staff), 1, staffFile)) {
							if((chosenStaff.passHash&0xFFFFFFFF00000000) != 0 && strcmp(chosenStaff.id, buf) == 0) {
								exists = true;
								break;
							}
						}
						rewind(staffFile);

						if(exists) {
							printf("A staff with the same ID exists!\n");
							pause();
						} else {
							strcpy(chosenStaff.id, buf);
						}
					}
				} else if(strcmp(buf, "NAME") == 0) {
					res = staffPromptDetails(buf, ~SE_NAME);
					if(res == 0) {
						strcpy(chosenStaff.details.name, buf);
					}
				} else if(strcmp(buf, "POSITION") == 0) {
					res = staffPromptDetails(buf, ~SE_POSITION);
					if(res == 0) {
						strcpy(chosenStaff.details.position, buf);
					}
				} else if(strcmp(buf, "PHONE") == 0) {
					res = staffPromptDetails(buf, ~SE_PHONE);
					if(res == 0) {
						strcpy(chosenStaff.details.phone, buf);
					}
				} else if(buf[0] == ':') {
					// String is in uppercase.
					if(buf[1] == 'Q') {
						res = -2;
					} else if(buf[1] == 'W') {
						break;
					} else if(buf[0] == 'H') {
						cls();
						printf(
							"Help:\n"
							"Type in field and value to replace to modify.\n"
							"Usage:\n"
							"\t$FIELD=$VALUE\n"
							"E.g.:\n"
							"\tName=John Smith\n\n"
						);
						pause();
						continue;
					}
				} else {
					printf("Entered field does not match any of the fields!\n");
					pause();
					continue;
				}

				if(res == EOF) {
					retval = EOF;
					goto CLEANUP;
				} else if(res == -2) {
					printf("Are you sure you want to abort? [yN]: ");
					res = scanf("%1c", buf);
					truncate();

					if(res == EOF) {
						retval = EOF;
						goto CLEANUP;
					} else if(buf[0] == 'Y') {
						retval = -2;
						goto CLEANUP;
					}
				}
			}

			printf("Are you sure you want to save this staff's details? [Yn]: ");
			if(scanf("%1c", buf) == EOF) {
				retval = EOF;
			}
			truncate();

			if(toupper(buf[0]) == 'N') {
				printf("Modify operation aborted!\n");
				pause();
				break;
			} else {
				// Go back one entry, since current pointer is at the end of the current Staff{}.
				fseek(staffFile, curFilePos-sizeof(Staff), SEEK_SET);
				fwrite(&chosenStaff, sizeof(Staff), 1, staffFile);
				++numModified;
				break;
			}
		} else {
			printf("Staff ID entered does not match any records!\n");
			pause();
		}
	}

CLEANUP:
	#undef ID_SIZE
	if(fclose(staffFile) != 0) {
		perror("Error (Closing staff file)");
		return -3;
	} else if(numModified != 0) {
		printf("Staff data modified successfully!\n");
		pause();
	}
	return retval;
}


int staffDisplay(void) {
	StaffDisplayOptions s = staffDisplayOptionsInit();
	s.header = 
		"DISPLAY STAFF\n"
		"=============\n";
	s.displayList[SE_ID] = true;
	s.displayList[SE_NAME] = true;
	s.displayList[SE_POSITION] = true;
	s.displayList[SE_PHONE] = true;
	s.isInclude = false;

	int res = staffDisplaySelected(&s);
	return res < 0 ? res : 0;
}


int staffReport(void) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "rb");

	if(staffFile == NULL) {
		perror("Error (opening staff file)");
		retval = -3;
		goto CLEANUP;
	}

	StaffDisplayOptions s = staffDisplayOptionsInit();
	s.header =
		"REPORT STAFF\n"
		"============\n";
	s.displayList[SE_ID] = true;
	s.displayList[SE_NAME] = true;
	s.displayList[SE_POSITION] = true;
	s.displayList[SE_PHONE] = true;
	s.displayDeleted = true;
	s.displayExisting = false;
	s.isInclude = false;

	int res = staffDisplaySelected(&s);
	if(res < 0) {
		retval = res;
		goto CLEANUP;
	}

CLEANUP:
	fclose(staffFile);
	return retval;
}


int staffDelete(void) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "r+b");
	Staff* staffArr = NULL;
	
	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		retval = -3;
		goto CLEANUP;
	}

	int size = -1;
	if(fseek(staffFile, 0, SEEK_END) != 0) {
		// fseek failed.
		perror("Error (fseek failed) ");
	} else {
		size = ftell(staffFile);
		rewind(staffFile);
	}

	int arrCapacity = size;
	if(size == -1) {
		// fseek failed. Set a default size for malloc().
		// Allocate memory that can fit 1024 entries of Staff{}. (around 200KB)
		arrCapacity = 1024;
	}
	// Try to load all of the Staff file data to memory so that a tmp file doesn't have to be created.
	staffArr = malloc(arrCapacity*sizeof(Staff));

	int len = 0;
	if(staffArr == NULL) {
		// malloc() failed.
		// TODO: 
	} else if(size != -1) {
		// malloc() suceeded and fseek suceeded.
		fread(staffArr, 1, size, staffFile);
		len = size/sizeof(Staff);
	} else {
		// malloc() suceeded but fseek failed.
		// Will have to extend malloc when needed.
		while(fread(staffArr+len, sizeof(Staff), 1, staffFile) == 1) {
			if(++len > 1024) {
				// TODO: Apply fix for realloc() fail.
				arrCapacity *= 2;
				staffArr = realloc(staffArr, arrCapacity);
			}
		}
	}
	rewind(staffFile);

	// Limit delete amount.
	// $ENTRIES_PER_PAGE to store user prompt.
	// $ID_SIZE + 1 to store exclamation mark.
	#define ID_SIZE 6
	char deleteListData[ENTRIES_PER_PAGE+1][ID_SIZE+1];
	// Create another array that contains the address of each string.
	char* deleteList[ENTRIES_PER_PAGE];
	for(int i = 0; i < ENTRIES_PER_PAGE; ++i) {
		deleteList[i] = &deleteListData[i][0];
	}


	// Options for staff selected for delete.
	StaffDisplayOptions delOpt = staffDisplayOptionsInit();
	delOpt.idList = deleteList;
	delOpt.displayList[SE_ID] = true;
	delOpt.displayList[SE_NAME] = true;
	delOpt.isInteractive = false;

	int* listCursor = &delOpt.idListLen;

	// Options for staffs remaining that have not been chosen for deletion.
	StaffDisplayOptions disOpt = staffDisplayOptionsInit();
	disOpt.idList = deleteList;
	disOpt.displayList[SE_ID] = true;
	disOpt.displayList[SE_NAME] = true;
	disOpt.isInclude = false;
	disOpt.isInteractive = false;

	while(1) {
		cls();
		printf(
			"DELETE STAFF\n"
			"============\n"
		);
		printf("Enter h for help.\n");

		if(*listCursor > 0) {
			// Auto discard value if it's invalid or repeaated.
			printf("Delete List\n");
			*listCursor = staffDisplaySelected(&delOpt);
			if(*listCursor == 0) {
				// Reprint again since table is empty.
				continue;
			} else if(*listCursor < 0) {
				retval = *listCursor;
				goto CLEANUP;
			}
			putchar('\n');
		}

		printf(
			"Employee List\n"
			"-------------\n"
		);

		disOpt.idListLen = *listCursor;
		int res = staffDisplaySelected(&disOpt);
		if(res < 0) {
			retval = res;
			goto CLEANUP;
		}

		printf("Enter command: ");

		res = scanf("%6s", deleteList[*listCursor]);
		truncate();

		if(res == EOF) {
			retval = EOF;
			goto CLEANUP;
		}

		if(toupper(deleteListData[*listCursor][0]) == 'W') {
			break;
		} else if(toupper(deleteListData[*listCursor][0]) == 'Q') {
			retval = -2;
			goto CLEANUP;
		} else if(toupper(deleteListData[*listCursor][0]) == 'H') {
			cls();
			printf(
				"HELP\n"
				"====\n"
				"Enter a staff's ID to put it into delete list.\n"
				"Prefix a staff ID with an exclamation mark to remove it from delete list.\n"
				"Actions:\n"
				"\tS0001  (Insert to delete list)\n"
				"\t!S0001 (Remove from delete list)\n"
				"\tp        (Previous page)\n"
				"\tn        (Next page)\n"
				"\tw        (Proceed to delete)\n"
				"\tq        (Abort delete)\n"
			);
			pause();
			continue;
		} else if(toupper(deleteListData[*listCursor][0]) == 'N') {
			// Out of bounds error are handled inside displaySelectedStaff();
			++disOpt.page;
		} else if (toupper(deleteListData[*listCursor][0]) == 'P') {
			// Out of bounds error are handled inside displaySelectedStaff();
			--disOpt.page;
		} else if(deleteList[*listCursor][0] == '!' && *listCursor > 0) {
			int i = 0;
			for(; i < *listCursor; ++i) {
				if(strcmp(deleteList[i], &deleteList[*listCursor][1]) == 0) {
					break;
				}
			}
			if(i != *listCursor) {
				// Matches.
				if(i+1 != *listCursor) {
					// Move (n-1)th staff to i.
					// Only do it if there are data after i.
					/* Demonstration:
								i			n (cursor)
						0	1	2	3	4	!2
						(to remove 2, move 4 (n-1 th element) to 2.)
								i			n
						0	1	4	3	4	!2
						(decrease cursor by one. to remove leftover and remove the remove command (!2).)
								i		n
						0	1	4	3	4	<uninit>
						(original 4 will be overwritten next loop)

						Q.E.D.
					*/
					memmove(deleteListData[i], deleteListData[*listCursor-1], ID_SIZE);
				}
				--*listCursor;
			}
			continue;
		} else if(*listCursor == ENTRIES_PER_PAGE) {
			printf(
				"You have reached the maximum entries limit per deletion request!\n"
				"To reduce the number of mistake that will occur, the deletion limit has been set to %d!\n", ENTRIES_PER_PAGE
			);
			pause();
			continue;
		}

		++*listCursor;
	}
	retval = *listCursor;
	
	// Modify staff's $passHash to zero.
	int acc = 0;
	for(int i = 0; i < len; ++i) {
		bool match = false;
		for(int ii = 0; ii < *listCursor; ++ii) {
			if((staffArr[i].passHash & 0xFFFFFFFF00000000) != 0  && strcmp(staffArr[i].id, deleteListData[ii]) == 0) {
				match = true;
				// Remove deleteListData[ii]. Staff ID is unique, so continue to next staff.
				deleteListData[ii][0] = '\0';
				break;
			}
		}

		if(match) {
			staffArr[i].passHash = 0;

			time_t rawTime;
			time(&rawTime);

			struct tm* time = localtime(&rawTime);
			staffArr[i].passHash |= (((short) time->tm_year)+1900) | (((char) time->tm_mon+1)<<16) | (((unsigned int) (char) time->tm_mday)<<24);

			fseek(staffFile, sizeof(Staff)*acc, SEEK_CUR);
			fwrite(&staffArr[i], sizeof(Staff), 1, staffFile);
			acc = 0;
		} else {
			++acc;
		}
	}
	if(*listCursor == 0) {
		printf("No staff record deleted!\n");
	} else if(*listCursor == 1) {
		printf("1 staff record deleted!\n");
	} else {
		printf("%d staff records deleted!\n", *listCursor);
	}
	pause();

CLEANUP:
	#undef ID_SIZE
	free(staffArr);

	if(fclose(staffFile) == EOF) {
		perror("Error (Closing staff file) ");
		retval = 1;
	}
	return retval;
}


int staffPromptDetails(char* buf, enum StaffModifiableFields selection) {
	char* promptMessage;
	//                127[^N]
	char format[] = "%       "; // XXX: Adjust accordingly if required.
	bool valid = false;

	bool isFlipped = false;
	selection = (selection&(1<<(sizeof(selection)*8-1))) != 0 ? isFlipped=true, ~selection : selection;

	switch(selection) {
		case SE_ID:
			promptMessage = "ID (S0001)";
			strcpy(format+1, "5s");
			break;
		case SE_NAME:
			promptMessage = "Name (John Smith)";
			strcpy(format+1, "127[^\n]");
			break;
		case SE_POSITION:
			promptMessage = "Position (Admin)";
			strcpy(format+1, "31[^\n]");
			break;
		case SE_PHONE:
			promptMessage = "Phone (0123456789)";
			strcpy(format+1, "15[^\n]");
			break;
		default:
			printf("Error (Unimplemented)\n");
			return -15;
	};

	while(!valid) {
		if(!isFlipped) {
			printf("Staff's %s: ", promptMessage);
		}

		if(scanf(format, buf) == EOF) {
			return EOF;
		}
		truncate();

		// Exit detection.
		if(buf[0] == ':' && (toupper(buf[1]) == 'Q' || toupper(buf[1]) == 'W')) {
			return -2;
		}

		valid = true;
		if(buf[0] == 0) {
			printf("Please enter a value!\n\n");
			valid = false;
		}

		switch(selection) {
			case SE_ID:
				if(buf[0] == 's') {
					valid = false;

					printf("Please ensure that the alphabet entered is in uppercase!\n\n");
					break;
				} else if(buf[0] != 'S') {
					valid = false;
					printf("Invalid Staff ID format!\n\n");
					break;
				}

				for(int i = 1; i < 5; ++i) {
					if(buf[i] < '0' || buf[i] > '9') {
						printf("Invalid Staff ID format!\n\n");
						valid = false;
						break;
					}
				}
				break;
			case SE_PHONE: { // Interesting... https://stackoverflow.com/questions/2036819/compile-error-with-switch-expected-expression-before
				int i = 0;
				for(; buf[i]; ++i) {
					if(buf[i] == '-') {
						printf("Please avoid putting hyphens\n\n");
						valid = false;
						break;
					} else if(buf[i] < '0' || buf[i] > '9') {
						printf("The phone number should only contain numbers\n\n");
						valid = false;
						break;
					}
				}

				if(valid && (i < 9 || i > 12)) {
					printf("Please enter a valid phone number!\n\n");
					valid = false;
				}
				break;
			}
			default:;
				// Do nothing.
		}
	}

	// No error.
	return 0;
}


StaffDisplayOptions staffDisplayOptionsInit(void) {
	return (StaffDisplayOptions) {
		NULL,
		NULL,
		{ 0, 0, 0, 0 },
		0,
		ENTRIES_PER_PAGE,
		0,
		true,
		true,
		false,
		true,
		{
			0,
			0,
			0
		}
	};
}


int staffDisplaySelected(StaffDisplayOptions* options) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "r");
	Staff* staffArr = NULL;
	char* includeFlag = NULL;
	int* arrCursorHist = NULL;

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	if(fseek(staffFile, 0, SEEK_END) != 0) {
		perror("Error (fseek staff file)");
		pause();
		// TODO: Design a backup solution along with malloc().
		retval = -3;
		goto CLEANUP;
	} else {
		int len = ftell(staffFile);
		if(len == -1) {
			// TODO: Design a backup solution.
			perror("Error (ftell staff file)");
			pause();
			retval = -3;
			goto CLEANUP;
		}
		rewind(staffFile);
		options->metadata.totalBytes = len;
		options->metadata.totalEntries = len / sizeof(Staff);
	}

	// Allocate memory to hold the whole file.
	staffArr = malloc(options->metadata.totalBytes*sizeof(Staff));
	if(staffArr == NULL) {
		perror("Error (malloc)");
		pause();
		// Designing a backup solution really will do me die ah.
		// TODO: Maybe if got time, remake a backup solution if malloc() fails.
		retval = -4;
		goto CLEANUP;
	}

	fread(staffArr, sizeof(Staff), options->metadata.totalEntries, staffFile);

	// If isInclude, set all flag to false, else true.
	// Include: Set all to exclude then build up include list.
	// Exclude: Set all to include then build up exclude list.
 	// A bitset to record which ID to print.
	if(options->isInclude) {
		includeFlag = calloc((options->metadata.totalEntries+7)/8, 1);
	} else {
		includeFlag = malloc((options->metadata.totalEntries+7)/8);
		if(includeFlag != NULL) {
			memset(includeFlag, ~0, (options->metadata.totalEntries+7)/8);
		}
	}
	
	if(includeFlag == NULL) {
		perror("Error (malloc bitset)");
		retval = -4;
		goto CLEANUP;
	}

	// Traverse the file and mark ith bit of $includeFlag as 0/1 to determine if it should be included/excluded from print.
	int total = 0; // Total number of matches (Will be printed).

	for(int i = 0; i < options->metadata.totalEntries; ++i) {
		if((staffArr[i].passHash & 0xFFFFFFFF00000000) == 0 ? !options->displayDeleted : !options->displayExisting) {
			// Hide staff.
			includeFlag[i/8] &= ~0 ^ (1 << (i%8));
			continue;
		}

		for(int ii = 0; ii < options->idListLen; ++ii) {
			/*	PROOF:
					Exclude: build up exclude. (Initially all include)
					Include: build up include. (Initially all exclude)

						(Mode)			(Match include/exclude list)
					isInclude		^ strcmp() != 0
					false (exclude)	^ true  (unmatched)	= true  (set as include)
					false (exclude)	^ false (matched)	= false (ignore)
					true  (include)	^ true  (unmatched)	= false (set as exclude)
					true  (include)	^ false (matched)	= true  (ignore)
				Q.E.D.
			*/
			// if(options->isInclude ^ (strcmp(staffArr[i].id, options->idList[ii]) != 0)) {
			if(strcmp(staffArr[i].id, options->idList[ii]) == 0) {
				// If exclude mode, set bit as 0.
				// If include mode, set bit as 1.
				if(!options->isInclude) {
					includeFlag[i/8] &= ~0 ^ (1 << (i%8));
				} else {
					includeFlag[i/8] |= (1 << (i%8));
					break; // Marked as include, don't need to check against other IDs.
				}
			} else if(!options->isInclude) {
				break;
			}
		}
	}

	for(int i = 0; i < options->metadata.totalEntries; ++i) {
		if((includeFlag[i/8]&(1<<(i%8))) != 0) {
			++total;
		}
	}
	options->metadata.matchedLength = total;

	if(options->page < 0) {
		options->page = 0;
	} else if(options->page * options->entriesPerPage > total) {
		options->page = options->metadata.matchedLength/options->entriesPerPage;
	}

	// Stores each starting position of the page. Indexed with $curPage.
	// Plus one to include zeroth page.
	arrCursorHist = malloc(sizeof(int*) * (1+(total+(options->entriesPerPage-1))/options->entriesPerPage));
	arrCursorHist[0] = 0;
	// XXX: Shift $arrCursorHist by 1 to make it a zero-based array.
	++arrCursorHist;

	int lastRead = 0;
	int read = 0;

	if(options->isInteractive) {
		cls();
	}

	while(1) {
		if(options->header != NULL) {
			printf("%s", options->header);
		}
		// ith index stores the width of dashes.
		// i+1th index stores the width of the column (including the dashes).
		short printWidth[STAFF_ENUM_LENGTH*2] = {
			8, 8,
			4, 50,
			8, 31,
			5, 12,
		};
		arrCursorHist[options->page] = arrCursorHist[options->page-1];

		int cols = 0;
		// Print headers.
		printf("Number  "); // Column of the number of current row.
		for(int i = 0; i < STAFF_ENUM_LENGTH; ++i) {
			if(!options->displayList[i]) {
				continue;
			}
			++cols;

			switch(i) {
				case SE_ID:
					printf("STAFF ID");
					break;
				case SE_NAME:
					// Allocate a width of 50 cols for name.
					// If a name exceeds 47 characters (excluding null), ellipsis will be added.
					printf("%-50s", "NAME");
					break;
				case SE_POSITION:
					printf("%-31s", "POSITION");
					break;
				case SE_PHONE:
					printf("%-12s", "PHONE"); 
					break;
				default:;
					// Do nothing.
			}
			printf("  ");
		}
		if(options->displayDeleted) {
			printf("DELETED");
		}
		putchar('\n');

		// Print header and content divider.
		printf("------  "); // Divider for number column.
		for(int i = 0; i < STAFF_ENUM_LENGTH*2; i += 2) {
			if(!options->displayList[i/2]) {
				continue;
			}

			int ii = 0;
			for(; ii < printWidth[i]; ++ii) {
				putchar('-');
			}
			// Plus 2 for row divider.
			for(; ii < printWidth[i+1]+2; ++ii) {
				putchar(' ');
			}
		}
		if(options->displayDeleted) {
			printf("-------");
		}
		putchar('\n');

		// Print staff details from staffArray.
		int arrCursor = arrCursorHist[options->page];
		for(; read < lastRead+options->entriesPerPage && read < total && arrCursor < options->metadata.totalEntries; ++arrCursor) {
			if((includeFlag[arrCursor/8]&(1<<(arrCursor%8))) == 0) {
				continue;
			}
			// Print column number.
			printf("%6d  ", ++read);

			for(int i = 0; i < STAFF_ENUM_LENGTH; ++i) {
				if(!options->displayList[i]) {
					continue;
				}
				switch(i) {
					case SE_ID:
						printf("%s     ", staffArr[arrCursor].id);
						break;
					case SE_NAME:
						printf("%-47s", staffArr[arrCursor].details.name);
						if(strlen(staffArr[arrCursor].details.name) > 47) {
							printf("...");
						}
						printf("     ");
						break;
					case SE_POSITION:
						printf("%-31s  ", staffArr[arrCursor].details.position);
						break;
					case SE_PHONE:
						printf("%-14s  ", staffArr[arrCursor].details.phone);
						break;
					default:;
						// Do nothing.
				}
			}

			if(options->displayDeleted) {
				if((staffArr[arrCursor].passHash & 0xFFFFFFFF00000000) == 0) {
					printf(
						"%04llu-%02llu-%02llu",
						staffArr[arrCursor].passHash&0xFFFF,
						(staffArr[arrCursor].passHash&0xFF0000)>>16,
						(staffArr[arrCursor].passHash&0xFF000000)>>24
					);
				} else {
					printf("False");
				}
			}
			putchar('\n');
		}
		// Print table border
		for(int i = 1; i < STAFF_ENUM_LENGTH*2; i += 2) {
			if(!options->displayList[i/2]) {
				continue;
			}
			for(int ii = printWidth[i]+2; ii; --ii) {
				putchar('-');
			}
		}
		printf("--------"); // Number column.
		if(options->displayDeleted) {
			printf("---------");
		}
		putchar('\n');

		if(total == 0) {
			printf("No matching entries!\n");
		}

		if(read-lastRead <= 1) {
			printf("Displaying %d entry", read-lastRead);
		} else {
			printf("Displaying %d entries", read-lastRead);
		}

		if(options->metadata.totalEntries >= 0) {
			printf(" of %d entr%s.", total, total < 2 ? "y" : "ies");		
		}
		printf(" (Page %d)\n", options->page+1);

		if(options->isInteractive) {
			printf("Enter N for next page, P to go to previous page, Q to quit (default=N): ");
			char action;

			if(scanf("%1c", &action) == EOF) {
				retval = EOF;
				goto CLEANUP;
			}
			truncate();

			if(toupper(action) == 'Q') {
				break;
			} else if(toupper(action) == 'P') {
				if(options->page > 1) {
					lastRead = lastRead < ENTRIES_PER_PAGE+(read-lastRead) ? 0 : lastRead-ENTRIES_PER_PAGE-(read-lastRead);
					read = lastRead;
					--options->page;
				} else {
					// Stay on the same page if on the 1st page.
					read = lastRead;
				}
			} else {
				if(read == total) {
					// Stay on the same page.
					read = lastRead;
				} else {
					// Do nothing since pointer is on the cur+1 page.
					lastRead = read;
					++options->page;
				}
			}
			cls();
		} else {
			putchar('\n');
			break;
		}
	}
	retval = total;

CLEANUP:
	free(staffArr);
	free(includeFlag);
	free(arrCursorHist-1);
	fclose(staffFile);
	return retval;
}


Staff staffLogin(void) {
	Staff s;
	char buf[STAFF_BUF_MAX];
	FILE* staffFile = fopen("staff.bin", "rb");

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		s.id[0] = 0;
		goto CLEANUP;
	}

	while(1) {
		// Any leftover characters after the 5th byte will be truncated.
		cls();
		printf(
			"LOGIN\n"
			"=====\n"
		);
		int res = staffPromptDetails(buf, SE_ID);
		if(res == EOF) {
			s.id[0] = EOF;
			goto CLEANUP;
		}

		if(res == -2) {
			// Abort.
			s.id[0] = 0;
			goto CLEANUP;
		}

		// Iterate through staff data and find the matching staff ID.
		bool found = false;

		rewind(staffFile);
		while(fread(&s, sizeof(s), 1, staffFile) != 0) {
			if(strcmp(s.id, buf) == 0) {
				// Found matching staff details, stop searching.
				found = true;
				break;
			}
		}

		if(found) {
			bool match = false;

			for(int i = 3; ~i; --i) {
				printf("Password: ");

				if(scanf("%127[^\n]", buf) == EOF) {
					s.id[0] = EOF;
					goto CLEANUP;
				}
				u64 enteredPassHash = computeHash(buf);
				memset(buf, 0, STAFF_BUF_MAX); // Zero out sensitive data.
				truncate();

				if(enteredPassHash == s.passHash) {
					// A valid password is entered.
					match = true;
					break;
				} else if(i > 0) {
					printf("Password incorrect! %d attempts left\n", i);
				}
			}

			if(match) {
				// Password matches, break out of login loop.
				printf("Logged in successfully.\n\n");
				pause();
				break;
			} else {
				printf("Password and staff ID does not match! Please try again!\n\n");
				pause();
			}
		} else {
			// Staff ID inputted not found in record, proceed to prompt again.
			printf("Staff ID not found!\n\n");
			pause();
		}
		
	}

CLEANUP:
	fclose(staffFile);

	return s;
}


// @retval	0	All good.
// @retval	EOF EOF received.
int staffMenu(void) {
	Staff loggedInUser;
	bool loggedIn = false;
	while(1) {
		cls();
		// TODO: Maybe a welcome user or something.
		// TODO: Make a better menu.
		char action[2];
		if(!loggedIn) {
			loggedInUser = staffLogin();
				
			if(loggedInUser.id[0] == EOF) {
				return EOF;
			} if(loggedInUser.id[0] != 0) {
				loggedIn = true;
			}
		} 
		if(loggedIn) {
			cls();
			printf(
				"STAFF MODULE\n"
				"============\n"
				"- Add\n"
				"- Display\n"
				"- Delete\n"
				"- Modify\n"
				"- Search\n"
				"- Report\n\n"
				"- Quit menu\n"
				"- Log out\n\n"
				"Enter a function: "
			);
		}

		if(scanf("%2s", action) == EOF) {
			truncate();
			return EOF;
		}
		truncate();

		if(loggedIn) {
			switch(toupper(*action)) {
				case 'A':
					staffAdd();
					break;
				case 'D':
					if(action[1] == '\0') {
						printf("\nPlease enter one more character (DI[SPLAY] / DE[LETE]) to distinguish between 2 similar functions!\n");
						pause();
						continue;
					}
					if(toupper(action[1]) == 'I') {
						staffDisplay();
					} else {
						staffDelete();
					}
					break;
				case 'M':
					staffModify();
					break;
				case 'S':
					staffSearch();
					break;
				case 'R':
					staffReport();
					break;
				case 'Q':
					return 0;
					// no break here because of return.
				case 'L':
					loggedIn = false;
					break;
				default:;
					// nothing
			}
		}
	}
	return 0;
}


u64 computeHash(char* msg) {
	// Define constants.
	#define NN	8	// Hash size to be returned (bytes).
	#define BB	128	// Block bytes.

	// Initialisation vector constants from the BLAKE2 specification.
	// Equivalent to the fractional part of the square root of the first 8 prime numbers.
	u64 hash[] = {
		0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
		0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
	};
	
	hash[0] ^= 0x01010000 ^ NN;

	int len = strlen(msg);

	// Split the message into $BB bytes each (and exclude the last block) and compress it.
	for(int i = 0; i < (len+BB-1)/BB-1; ++i) {
		BLAKE2bF(hash, &msg[i*BB], (i+1)*BB, false);
	}

	// Compress final block.
	char buf[BB] = { 0 };
	// NOTE: Any bytes beyond the null terminator will not be used.
	memcpy(buf, &msg[len/BB*BB], len%BB);

	BLAKE2bF(hash, buf, len, true);
	memset(buf, 0, BB); // Erase sensitive data.

	#undef NN
	#undef BB

	return hash[0];
}


void BLAKE2bF(u64* hash, char* msg, u64 bytesCompressed, bool isLastBlock) {
	// Sigma array taken from the BLAKE2 specification to determine which index of the msg to take and mix.
	// Row 11 and 12 are equivalent to row 1 and 2.
	const char SIGMA[12][16] = {
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
		{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
		{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
		{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
		{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
		{ 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
		{ 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
		{ 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
		{ 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }
	};
	// Number of rounds to mix the message specified for BLAKE2b.
	const int ROUNDS = 12;

	u64* m = (u64*) msg; // Interpret 128 bytes msg to a 16 chunks 8 bytes (64 bits) int.

	u64 v[16] = {
		hash[0], hash[1], hash[2], hash[3],
		hash[4], hash[5], hash[6], hash[7],
		0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
		0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
	};

	v[12] ^= bytesCompressed;
	// v[13] ^= bytesCompressed<<64; // Ignored because size of $bytesCompressed is limited to 8 bytes in this implementation.

	if(isLastBlock) {
		v[14] = ~v[14]; // Inverts all bits
	}

	for(int i = 0; i < ROUNDS; ++i) {
		// Choose the specified sigma array for the current round.
		const char* S = SIGMA[i];

		BLAKE2bG(v, 0, 4,  8, 12, m[S[ 0]], m[S[ 1]]);
		BLAKE2bG(v, 1, 5,  9, 13, m[S[ 2]], m[S[ 3]]);
		BLAKE2bG(v, 2, 6, 10, 14, m[S[ 4]], m[S[ 5]]);
		BLAKE2bG(v, 3, 7, 11, 15, m[S[ 6]], m[S[ 7]]);

		BLAKE2bG(v, 0, 5, 10, 15, m[S[ 8]], m[S[ 9]]);
		BLAKE2bG(v, 1, 6, 11, 12, m[S[10]], m[S[11]]);
		BLAKE2bG(v, 2, 7,  8, 13, m[S[12]], m[S[13]]);
		BLAKE2bG(v, 3, 4,  9, 14, m[S[14]], m[S[15]]);
	}

	for(int i = 0; i < 8; ++i) {
		hash[i] ^= v[i] ^ v[i+8];
	}
}


void BLAKE2bG(u64* v, u64 a, u64 b, u64 c, u64 d, u64 x, u64 y) {
	#define rotateRight64Bits(a,n) ((a)>>(n))^((a)<<(64-(n)))

	v[a] += v[b] + x;
	v[d] = rotateRight64Bits(v[d]^v[a], 32);
	v[c] += v[d];
	v[b] = rotateRight64Bits(v[b]^v[c], 24);
	v[a] += v[b] + y;
	v[d] = rotateRight64Bits(v[d]^v[a], 16);
	v[c] += v[d];
	v[b] = rotateRight64Bits(v[b]^v[c], 63);

	#undef rotateRight64Bits
}


int KMPSearch(char* text, char* query) {
	int LPS[STAFF_BUF_MAX-1] = { 0 };
	int textLen = strlen(text);
	int queryLen = strlen(query);

	// Build array.
	for(int r = 1, l = 0; r < queryLen; ++r) {
		if(query[r] == query[l]) {
			LPS[r] = ++l;
		} else {
			l = 0;
		}
	}

	int qIdx = 0;
	int i = 0;
	while(i < textLen) {
		if(qIdx == queryLen) {
			return i-queryLen+1;
		}

		if(query[qIdx] == text[i]) {
			++qIdx;
			++i;
		} else {
			qIdx = LPS[qIdx-1];
			if(qIdx == 0) {
				++i;
			}
		}
	}

	return -1;
}


int main(void) {
	Staff a = {"S0000", {"ADMIN", "ADMIN", "0123456789"}, computeHash("ADMIN")};
	if(staffMenu() == EOF) {
		FILE* file = fopen("staff.bin", "wb");
		fwrite(&a, sizeof(a), 1, file);
		fclose(file);
	}
	return 0;
}

#undef STAFF_ENUM_LENGTH
#undef STAFF_BUF_MAX
#undef ENTRIES_PER_PAGE
#undef truncate
#undef pause
#undef ENABLE_CLS
#undef cls