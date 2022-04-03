#include<ctype.h>	// toupper()
#include<stdbool.h>	// bool, true, false
#include<stdio.h>	// fclose(), fopen(), fread(), fseek(), ftell(), fwrite(), getchar(), perror(), printf(), rewind(), scanf(), ungetc(), EOF, FILE, SEEK_END, stdin
#include<stdlib.h>	// atoi(), calloc(), free(), malloc(), realloc()
#include<string.h>	// memmove(), memset(), strcmp(), strcpy(), strlen()
#include<time.h>	// localtime(), time(), time_t, struct tm


typedef unsigned long long u64;


/*
	This enum list all the modifiable fields found in the Staff{}.
	This enum will be used in promptStaffDetails() to determine which field is going to be modified.

	XXX: Maximum 127 enums if it's 1 byte. Sign bit is used for other purposes.
*/
enum StaffModifiableFields { SE_ID, SE_NAME, SE_POSITION, SE_PHONE, SE_IC };
#define STAFF_ENUM_LENGTH 5


// Define maximum char array size needed for Staff struct elements buffer.
#define STAFF_BUF_MAX 128

// Define how many rows of records to display in displayStaff().
#define ENTRIES_PER_PAGE 8

// Define a small macro to check if a staff is deleted.
#define isStaffDeleted(_staff) (((_staff).passHash&0xFFFFFFFF00000000) == 0)

// Define a small function to truncate remaining bytes in stdin.
#define truncate()													\
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
#define ENABLE_CLS true

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
	} while(0)

/*
	Data dictionary
	===============
	Name			Example				Description
	----			------				-----------
	ID				S0001				An unique ID assigned to the staff.
	name			John Smith			Stores the name of the staff.
	position		Admin				A string that records the position of the staff.
	phone			0123456789			A string that stores the phone number of the staff. Defaults to MYS country code of +60 implicitly.
	ic				000101070000		A string that stores the IC number of the staff.
	passHash		<64 bits binary>	A read-only hash of the staff's password.

	XXX:	passHash will be set to 0 if the staff was deleted.
			Upon setting to 0, the first 32 bits will be used for date logging.
			32 bits|8 bits|8 bits|16 bits (Big endian layout as an example)
			zeroes  day    month  year

	NOTE:	Modify $STAFF_BUF_MAX as well if the largest buffer size is changed (current max, $name, is 128 bytes).
*/
typedef struct {
	char id[6];
	struct {
		char name[128];
		char position[32];
		char phone[16];
		char ic[15];
	} details;
	// 3 bytes padding.
	u64 passHash;
} Staff;


// Instead of initialising this with the normal struct initialisation,
// get a copy of this from DisplayStaffOptionsInit(), filled with default values.
// Then only modify the fields' value.
typedef struct {
	char* header;							// Message to pass in to retain printed contents if isInteractive is true.
	char** idList;							// A char array that contains a 6 bytes string staff ID to include/exclude.
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
} DisplayStaffOptions;


// ----- START OF HEADERS -----
/*
	Error codes:
		> 0		No error.
		-1/EOF	EOF signal received.
		-2		User aborted the operation.
		-3		File/stdio error.
		-4		Heap allocation/stdlib error
		-n		Miscellaneous errors.
*/

/**
 * @brief	Presents a screen to add more staff to the staff record.
 *
 * @retval	0	Staff successfully added.
 * @retval	EOF	Staff addition cancelled (EOF signal received).
 * @retval	-2	Staff addition cancelled (Add operation was cancelled by user).
 * @retval	-3	Staff failed to be added (File operation error).
 */
int addStaff(void);


/**
 * @brief	Presents a screen to search for staffs that match a pattern in staff record.
 *
 * This function is used for searching through staff details that match a certain pattern.
 * It is case insensitive and allow the usage of SQL's LIKE's wildcard like pattern (%, _).
 *
 * @retval	0	Staff file successfully searched.
 * @retval	EOF	Staff search cancelled (EOF signal received).
 * @retval	-2	Staff search cancelled (Search operation was cancelled by user).
 * @retval	-3	Staff search failed (File operation error).
 */
int searchStaff(void);


/**
 * @brief	Select a staff to modify their details.
 *
 * @retval	0	Staff successfully modified.
 * @retval	EOF	Staff modification cancelled (EOF signal received).
 * @retval	-2	Staff modification cancelled (Modify operation was cancelled by user).
 * @retval	-3	Staff failed to be modified (File operation error).
 */
int modifyStaff(void);


/**
 * @brief	Presents a screen with all the member details in a interactive table format.
 *
 * @retval	0	Staffs details successfully displayed.
 * @return		Negative value indicating error. (See displaySelectedStaff() error codes)
 */
int displayStaff(void);


/**
 * @brief	Present a screen that show past employee with a brief summary.
 *
 * @retval	0	Staffs details successfully displayed and quit normally.
 * @retval	EOF	Staff report opertion aborted (EOF received).
 * @retval	-3	Staff failed to be displayed (File operation error).
 */
int reportStaff(void);

/**
 * @brief	Select staffs to delete.
 *
 * @retval	EOF	Staff deletion failed (EOF signal received).
 * @retval	-15	Staff deletion failed (An error occured).
 * @return		Number of staffs deleted.
 */
int deleteStaff(void);


/**
 * @brief	Prompts the user to enter selected staff details and validate them.
 *
 * This function handles the user input by validating them for correctness.
 * The inputted data will be truncated to the appropriate size to avoid buffer overflow.
 * This function makes the assumption that $buf is appropriately sized (>= to the size of corresponding field in Staff{}).
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
 * @retval	-14	Passing back control to callee (to reprint prompt) if bit flipped $selection is passed in.
 * @retval	-15	Read but not validated (Not implemented).
 */
int promptStaffDetails(char* buf, enum StaffModifiableFields selection);


/**
 * @brief	Returns a copy of a DisplayStaffOptions{} filled with the default values.
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
DisplayStaffOptions displayStaffOptionsInit(void);


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
int displaySelectedStaff(DisplayStaffOptions* options);


/**
 * @brief	Presents the user with a login screen and prompts the user to login.
 *
 * A login screen that allows staffs to login.
 * The staff's ID will be used to determine which staff is planning to login.
 * If the staff's ID is valid and matched one of the records, a password will be prompted.
 * If the inputted password is incorrect thrice, staff ID will be prompted again in case the staff ID was entered incorrectly.
 * A complete Staff struct with the corresponding staff details will be returned, after the staff has logged in.
 *
 * @return	First byte of ID is filled with 0 to indicate error, the next byte will be filled with the error code.
 * @return	Logged in user's Staff struct.
 */
Staff loginStaff(void);


/**
 * @brief	Presents the user with a screen to choose which staff function to perform.
 *
 * @param	loggedInUser	A pointer to the logged in user's struct.
 *
 * @retval	0	Quit normally.
 * @retval	EOF	EOF signal received.
 */
int menuStaff(Staff* loggedInUser);


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
 * @param	msg	A pointer to a char array to be hashed.
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
 * @param	text		Text to be searched.
 * @param	query		Query to search the text provided.
 * @param	ignoreCase	Determine to be case sensitive or not.
 *
 * @retval	-1	No match.
 * @return		Index of the first occurence.
 */
int KMPSearch(char* text, char* query, bool ignoreCase);


/**
 * @brief	Matches $text and $query and see if they are the same.
 *
 * This function allows the use of SQL's LIKE's wildcards to match.
 * Wildcards:
 *   *	Matches zero or more characters.
 *   _	Matches exactly on character.
 *
 * @param	text	The text to match against.
 * @param	query	The text that allow the use of wildcards.
 * @param	ignoreCase	Determine to be case sensitive or not.
 *
 * @return 	A true or false value indicating if they match.
 */
bool LIKE(char* text, char* query, bool ignoreCase);
// ----- END OF HEADERS -----


int addStaff(void) {
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
		if(curPrompt > SE_IC) {
			printf("Staff IC       : %s\n", newStaff.details.ic);
		}

		if(curPrompt > SE_ID) {
			// Insert a line break to separate from the table.
			putchar('\n');
		}
		printf("(Enter ':q' to abort.)\n\n");

		int res = -1; // Records the output of promptStaffDetails().
		switch(curPrompt) {
			case SE_ID:
				while(res != 0) {
					res = promptStaffDetails(buf, SE_ID);
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
						if(strcmp(tmp.id, buf) == 0 && !isStaffDeleted(tmp)) {
							exists = true;
							break;
						}
					}
					rewind(staffFile);

					if(exists) {
						printf("Staff with the same ID exists!\n\n");
						res = -15;
					} else {
						strcpy(newStaff.id, buf);
						res = 0;
					}
				}
				break;
			case SE_NAME:
				res = promptStaffDetails(buf, SE_NAME);
				if(res == 0) {
					strcpy(newStaff.details.name, buf);
				}
				break;
			case SE_POSITION:
				res = promptStaffDetails(buf, SE_POSITION);
				if(res == 0) {
					strcpy(newStaff.details.position, buf);
				}
				break;
			case SE_PHONE:
				res = promptStaffDetails(buf, SE_PHONE);
				if(res == 0) {
					strcpy(newStaff.details.phone, buf);
				}
				break;
			case SE_IC:
				res = promptStaffDetails(buf, SE_IC);
				if(res == 0) {
					strcpy(newStaff.details.ic, buf);
				}
				break;
			default: {
				res = 0;
			}
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
		printf("Password (E.g. Ky54asdf' _.): ");
		
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
			printf("Please enter a password that is shorter than %d characters!\n", STAFF_BUF_MAX-1);
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
		memset(buf, 0, STAFF_BUF_MAX-1); // null terminator is already zero, don't have to zero it again.
		truncate();

		if(hash1 == hash2) {
			newStaff.passHash = hash1;
			break;
		} else {
			printf("Passwords do not match!\n\n");
		}
	}

	printf("\nAre you sure you want to save the current staff detail? [Y/n]: ");
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
	if(staffFile != NULL && fclose(staffFile) == EOF) {
		perror("Error (Closing staff file) ");
		pause();
		retval = -3;
	} else if(retval == 0) {
		printf("New staff details saved successfully!\n");
		pause();
		printf("Do you want to add another record? [Y/n]: ");

		if(scanf("%c", buf) == EOF) {
			retval = EOF;
		} else {
			truncate();
			if(toupper(buf[0]) == 'N') {
				printf("User chose to not add another record!\n");
			} else {
				// Hopefully tail call optimisation is performed...
				return addStaff();
			}
		}
	}

	return retval;
}


int searchStaff(void) {
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
	rewind(staffFile); // Resets file cursor after fseek()-ed.

	if(len == -1L) {
		perror("Error (ftell)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	staffArr = malloc(len);
	if(staffArr == NULL) {
		perror("Error (malloc)");
		pause();
		retval = -4;
		goto CLEANUP;
	}

	if(fread(staffArr, len, 1, staffFile) != 1) {
		perror("Error (fread)");
		pause();
		retval = -3;
		goto CLEANUP;
	}
	rewind(staffFile);

	// Set up array to keep matches.
	#define ID_SIZE 6

	DisplayStaffOptions opt = displayStaffOptionsInit();

	int curCapacity = 128;
	// NOTE: Code that determines if a record should be inserted depends on each string being ID_SIZE-d, please change the code if this has changed.
	matches = malloc(ID_SIZE*curCapacity);
	matchesPtr = malloc(curCapacity*sizeof(char*));

	if(matches == NULL) {
		perror("Error (malloc $matches)");
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

	// Include all during first print.
	for(int i = 0; i < len/(int)sizeof(Staff); ++i) {
		if(!isStaffDeleted(staffArr[i])) {
			// XXX: Assuming matches is ID_SIZE sized.
			strcpy(&matches[opt.idListLen++*6], staffArr[i].id);
		}
	}
	int* matchesLen = &opt.idListLen;

	for(int i = 0; i < curCapacity; ++i) {
		// XXX: Assuming matches is ID_SIZE sized.
		matchesPtr[i] = matches+i*ID_SIZE;
	}
	opt.idList = matchesPtr;
	opt.displayList[SE_NAME] = true;
	opt.displayList[SE_ID] = true;
	opt.displayList[SE_POSITION] = true;
	opt.displayList[SE_PHONE] = true;
	opt.displayList[SE_IC] = true;

	opt.isInteractive = false;

	char buf[STAFF_BUF_MAX];

	while(1) {
		cls();
		printf(
			"SEARCH STAFF\n"
			"============\n"
		);
		displaySelectedStaff(&opt);

		printf(
			"(Enter ':h' for help.)\n"
			"(Enter ':q' for quit.)\n"
			"(Field[!/+/-]=Query): "
		);
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

		// Checks if user wants to invert search.
		bool invertSearch = false;
		if(buf[strlen(buf)-1] == '!' && buf[0] != ':') {
			invertSearch = true;
			buf[strlen(buf)-1] = 0;
		}

		// Checks if user wants to append.
		bool appendSearch = false;
		if(buf[strlen(buf)-1] == '+') {
			appendSearch = true;
			buf[strlen(buf)-1] = 0;
		}

		// Checks if user wants to remove matches.
		bool removeSearch = false;
		if(buf[strlen(buf)-1] == '-') {
			removeSearch = true;
			buf[strlen(buf)-1] = 0;
		}

		if(buf[0] == ':') {
			if(buf[1] == 'Q' || buf[1] == 'W') {
				retval = -2;
				goto CLEANUP;
			} else if(buf[1] == 'H') {
				cls();
				printf(
					"Help\n"
					"====\n"
					"  Type in field name and search query to search.\n"
					"  The search query may contain special characters such as:\n"
					"    '%%' to match zero or more characters.\n"
					"    '_' to match any character.\n"
					"  Actions:\n"
					"    $FIELD=$QUERY  (Display staffs that match with $QUERY.)\n"
					"    $FIELD!=$QUERY (Display staffs that does not match with $QUERY.)\n"
					"    $FIELD+=$QUERY (Append staffs that match with $QUERY to display list.)\n"
					"    $FIELD-=$QUERY (Remove staffs that match with $QUERY in display list.)\n"
					"    :h             (Help.)\n"
					"    :q             (Quit.)\n"
					"    :n             (Next page.)\n"
					"    :b             (Go back a page.)\n"
					"  Examples:\n"
					"    Name=J%%\n"
					"    (This searches for any name that starts with a capital 'J'.)\n"
					"    Phone!=01%%\n"
					"    (This searches for any phone that does not starts with '01'.)\n\n"
				);
				pause();
			} else if(buf[1] == 'N') {
				++opt.page;
			} else if(buf[1] == 'B') {
				--opt.page;
			} else {
				printf("Invalid control code!\n");
				pause();
			}
			continue;
		} else if(strcmp(buf, "ID") == 0) {
			field = SE_ID;
		} else if(strcmp(buf, "NAME") == 0) {
			field = SE_NAME;
		} else if(strcmp(buf, "POSITION") == 0) {
			field = SE_POSITION;
		} else if(strcmp(buf, "PHONE") == 0) {
			field = SE_PHONE;
		} else if(strcmp(buf, "IC") == 0) {
			field = SE_IC;
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


		// Will only enter here if a field is matched.

		int queryLen = strlen(buf);

		if(!appendSearch && !removeSearch) {
			*matchesLen = 0; // Reset to zero since it's not adding or removing from the search.
		}

		for(int i = 0; i < (int) (len/sizeof(Staff)); ++i) {
			if(!isStaffDeleted(staffArr[i])) {
				char* text = "";

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
					case SE_IC:
						text = staffArr[i].details.ic;
						break;
					default:
						retval = -15;
						break;
				}

				bool insert = !invertSearch;
				if(LIKE(text, buf, true)) {
					if(appendSearch || removeSearch) {
						for(int ii = 0; ii < *matchesLen; ++ii) {
							if(strcmp(matchesPtr[ii], staffArr[i].id) == 0) {
								if(removeSearch) {
									if(ii+1 != *matchesLen) {
										// XXX: Making the assumption each ID is ID_SIZE and no larger or smaller.
										// Please change this if the size of string has changed.
										// Using the same idea as deleteStaff();
										strcpy(matchesPtr[ii], matchesPtr[*matchesLen-1]);
									}
									--*matchesLen; // SAFETY: $matchesLen is guaranteed to be positive non-zero here.
								}
								// Skips appending if it's a duplicate.
								insert = false;
								break;
							}
						}
						if(removeSearch) {
							insert = false;
						}
					}
				} else {
					insert = invertSearch;
				}

				if(insert) {
					if(*matchesLen == curCapacity) {
						curCapacity *= 2;
						matches = realloc(matches, curCapacity*ID_SIZE);
						matchesPtr = realloc(matchesPtr, curCapacity*sizeof(char*));
						opt.idList = matchesPtr;

						if(matches == NULL || matchesPtr == NULL) {
							perror("Error (realloc)");
							pause();
							retval = -4;
							goto CLEANUP;
						} else {
							// $matches may have moved to another location, so start again from 0.
							for(int i = 0; i < curCapacity; ++i) {
								matchesPtr[i] = matches + i*ID_SIZE;
							}
						}
					}
					strcpy(matchesPtr[(*matchesLen)++], staffArr[i].id);
				}
			}
		}
		buf[queryLen] = 0;
	}

CLEANUP:
	#undef ID_SIZE
	if(staffFile != NULL) {
		fclose(staffFile);
	}
	free(staffArr);
	free(matches);
	free(matchesPtr);
	return retval;
}


int modifyStaff(void) {
	int retval = 0;
	int numModified = 0;
	FILE* staffFile = fopen("staff.bin", "rb+");
	
	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	Staff chosenStaff;
	
	char buf[STAFF_BUF_MAX];
	char id[6];

	while(1) {
		cls();
		printf(
			"MODIFY STAFF\n"
			"============\n"
		);
	
		printf("Type a staff ID to modify their staff details or ':q' to quit.\n\n");
		int res = promptStaffDetails(id, SE_ID);
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
			if(strcmp(chosenStaff.id, id) == 0 && !isStaffDeleted(chosenStaff)) {
				found = true;
				break;
			}
		}

		int curFilePos = ftell(staffFile); // Records current location to start writing modified record later from here (and rewind back a record).
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
					"ID       : %s\n"
					"Name     : %s\n"
					"Position : %s\n"
					"Phone    : %s\n"
					"IC       : %s\n\n"
					"(Enter ':h' for help.)\n"
					"(Enter ':q' for quit.)\n"
					"(Enter ':w' for save.)\n"
					"(Field=Value): ",
					chosenStaff.id, chosenStaff.details.name, chosenStaff.details.position, chosenStaff.details.phone, chosenStaff.details.ic
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
					res = promptStaffDetails(buf, ~SE_ID);
					if(res == 0) {
						bool exists = false;
						while(fread(&chosenStaff, sizeof(Staff), 1, staffFile)) {
							if(!isStaffDeleted(chosenStaff) && strcmp(chosenStaff.id, buf) == 0) {
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
					res = promptStaffDetails(buf, ~SE_NAME);
					if(res == 0) {
						strcpy(chosenStaff.details.name, buf);
					}
				} else if(strcmp(buf, "POSITION") == 0) {
					res = promptStaffDetails(buf, ~SE_POSITION);
					if(res == 0) {
						strcpy(chosenStaff.details.position, buf);
					}
				} else if(strcmp(buf, "PHONE") == 0) {
					res = promptStaffDetails(buf, ~SE_PHONE);
					if(res == 0) {
						strcpy(chosenStaff.details.phone, buf);
					}
				} else if(strcmp(buf, "IC") == 0) {
					res = promptStaffDetails(buf, ~SE_IC);
					if(res == 0) {
						strcpy(chosenStaff.details.ic, buf);
					}
				} else if(buf[0] == ':') {
					// String is in uppercase.
					if(buf[1] == 'Q') {
						res = -2;
					} else if(buf[1] == 'W') {
						break;
					} else if(buf[1] == 'H') {
						cls();
						printf(
							"Help\n"
							"====\n"
							"  Type in field and value to replace to modify.\n"
							"  Actions:\n"
							"    $FIELD=$VALUE (Modify the value of $FIELD.)\n"
							"    :h            (Help.)\n"
							"    :q            (Abort modifications.)\n"
							"    :w            (Save modifications.)\n"
							"  Example:\n"
							"    Name=John Smith\n\n"
						);
						pause();
						continue;
					} else {
						printf("Invalid control code!\n");
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
					printf("Are you sure you want to abort? [y/N]: ");
					res = scanf("%c", buf);
					truncate();

					if(res == EOF) {
						retval = EOF;
						goto CLEANUP;
					} else if(toupper(buf[0]) == 'Y') {
						retval = -2;
						goto CLEANUP;
					}
				} else if(res == -14) {
					// Add pause to see what error message was printed.
					pause();
				}
			}

			printf("Are you sure you want to save this staff's details? [Y/n]: ");
			if(scanf("%c", buf) == EOF) {
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
	if(staffFile != NULL && fclose(staffFile) != 0) {
		perror("Error (Closing staff file, file data might not be saved)");
		pause();
		return -3;
	} else if(numModified != 0) {
		printf("Staff data modified successfully!\n");
		pause();
	}
	return retval;
}


int displayStaff(void) {
	DisplayStaffOptions s = displayStaffOptionsInit();
	s.header =
		"DISPLAY STAFF\n"
		"=============\n";
	s.displayList[SE_ID] = true;
	s.displayList[SE_NAME] = true;
	s.displayList[SE_POSITION] = true;
	s.displayList[SE_PHONE] = true;
	s.displayList[SE_IC] = true;
	s.isInclude = false;

	int res = displaySelectedStaff(&s);
	return res < 0 ? res : 0;
}


int reportStaff(void) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "rb");

	if(staffFile == NULL) {
		perror("Error (opening staff file)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	DisplayStaffOptions s = displayStaffOptionsInit();
	s.header =
		"REPORT STAFF\n"
		"============\n";
	s.displayList[SE_ID] = true;
	s.displayList[SE_NAME] = true;
	s.displayList[SE_POSITION] = true;
	s.displayList[SE_PHONE] = true;
	s.displayList[SE_IC] = true;
	s.displayDeleted = true;
	s.displayExisting = true;
	s.isInclude = false;

	int res = displaySelectedStaff(&s);
	if(res < 0) {
		retval = res;
		goto CLEANUP;
	}

CLEANUP:
	if(staffFile != NULL) {
		fclose(staffFile);
	}
	return retval;
}


int deleteStaff(void) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "rb+");
	Staff* staffArr = NULL;
	
	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		pause();
		retval = -3;
		goto CLEANUP;
	}

	int size = -1;
	if(fseek(staffFile, 0, SEEK_END) != 0) {
		// fseek failed.
		perror("Error (fseek failed) ");
		pause();
	} else {
		size = ftell(staffFile);
		rewind(staffFile);
	}

	int arrCapacity = size;
	if(size == -1) {
		// fseek failed. Set a default size for malloc().
		// Allocate memory that can fit 128 entries of Staff{}. (26KiB)
		arrCapacity = 128;
	}
	staffArr = malloc(arrCapacity*sizeof(Staff));

	int len = 0;
	if(staffArr == NULL) {
		// malloc() failed.
		perror("Error (malloc failed)");
		retval = -4;
		goto CLEANUP;
	} else if(size != -1) {
		// malloc() suceeded and fseek suceeded.
		fread(staffArr, 1, size, staffFile);
		len = size/sizeof(Staff);
	} else {
		// malloc() suceeded but fseek failed.
		while(fread(staffArr+len, sizeof(Staff), 1, staffFile) == 1) {
			if(++len > arrCapacity) {
				arrCapacity *= 2;
				staffArr = realloc(staffArr, arrCapacity*sizeof(Staff));

				if(staffArr == NULL) {
					perror("Error (realloc failed)");
					retval = -4;
					goto CLEANUP;
				}
			}
		}
	}
	rewind(staffFile);

	// Limit delete amount.
	// $ENTRIES_PER_PAGE + 1 to store user prompt.
	// $ID_SIZE + 1 to check for overflows.
	// $ID_SIZE + 1 to store exclamation mark.
	#define ID_SIZE 6
	char deleteListData[ENTRIES_PER_PAGE+1][ID_SIZE+2];
	// Create another array that contains the address of each string.
	char* deleteList[ENTRIES_PER_PAGE+1];
	for(int i = 0; i < ENTRIES_PER_PAGE+1; ++i) {
		deleteList[i] = &deleteListData[i][0];
	}


	// Options for staff selected for delete.
	DisplayStaffOptions delOpt = displayStaffOptionsInit();
	delOpt.idList = deleteList;
	delOpt.displayList[SE_ID] = true;
	delOpt.displayList[SE_NAME] = true;
	delOpt.isInteractive = false;

	int* listCursor = &delOpt.idListLen;

	// Options for staff remaining that have not been chosen for deletion.
	DisplayStaffOptions disOpt = displayStaffOptionsInit();
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

		if(*listCursor > 0) {
			printf(
				"Delete List:\n"
				"------------\n"
			);

			// Auto discard value if it's invalid or repeaated.
			int displayed = displaySelectedStaff(&delOpt);
			// In case where listCuror is at max and should not be incresed.
			if(displayed < *listCursor) {
				*listCursor = displayed;
			}

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
			"Employee List:\n"
			"--------------\n"
		);

		disOpt.idListLen = *listCursor;
		int res = displaySelectedStaff(&disOpt);
		if(res < 0) {
			retval = res;
			goto CLEANUP;
		}

		printf(
			"(Enter ':h' for help.)\n"
			"(Enter ':q' for help.)\n"
			"Enter command: "
		);

		res = scanf("%7s", deleteList[*listCursor]);

		if(res == EOF) {
			retval = EOF;
			goto CLEANUP;
		}
		truncate();

		// Check for overflow.
		int inputLen = strlen(deleteList[*listCursor]);
		if(deleteList[*listCursor][0] == '!' ? inputLen > ID_SIZE : inputLen > ID_SIZE-1) {
			printf("Please enter a valid ID!\n");
			pause();
			continue;
		}

		// Check if it's numbers only.
		bool isNumber = true;
		for(char* i = deleteList[*listCursor]; *i; ++i) {
			if(*i < '0' || *i > '9') {
				isNumber = false;
				break;
			}
		}

		int selected = -1;
		if(isNumber && atoi(deleteList[*listCursor]) <= len) {
			selected = atoi(deleteList[*listCursor]);
		}
		if(selected == 0) {
			selected = -1;
		}

		if(deleteListData[*listCursor][0] == ':') {
			if(toupper(deleteListData[*listCursor][1]) == 'W') {
				break;
			} else if(toupper(deleteListData[*listCursor][1]) == 'Q') {
				retval = -2;
				goto CLEANUP;
			} else if(toupper(deleteListData[*listCursor][1]) == 'H') {
				cls();
				printf(
					"HELP\n"
					"====\n"
					"  Enter a staff's ID to put it into delete list.\n"
					"  Prefix a staff ID with an exclamation mark to remove it from delete list.\n"
					"  Actions:\n"
					"    S0001  (Insert to delete list.)\n"
					"    !S0001 (Remove from delete list.)\n"
					"    2      (Insert staff on row 2 to delete list.)\n"
					"    :h     (Help.)\n"
					"    :b     (Go back a page.)\n"
					"    :n     (Next page.)\n"
					"    :w     (Proceed to delete.)\n"
					"    :q     (Abort delete.)\n\n"
				);
				pause();
				continue;
			} else if(toupper(deleteListData[*listCursor][1]) == 'N') {
				// Out of bounds error are handled inside displaySelectedStaff();
				++disOpt.page;
			} else if (toupper(deleteListData[*listCursor][1]) == 'B') {
				// Out of bounds error are handled inside displaySelectedStaff();
				--disOpt.page;
			} else {
				printf("Invalid control code!\n");
				pause();
			}
			continue;
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
		} else if(*listCursor >= ENTRIES_PER_PAGE) {
			printf(
				"\n"
				"You have reached the maximum entries limit per deletion request!\n"
				"To reduce the number of mistake that will occur, the deletion limit has been set to %d!\n", ENTRIES_PER_PAGE
			);
			pause();
			continue; // So that $listCursor is not incremented.
		} else if(selected != -1) {
			// $acc should be guaranteed to be within bounds, I think.
			int acc = 0;
			int i = 0;
			for(; acc != selected; ++i) {
				if(!isStaffDeleted(staffArr[i])) {
					// Check if the current staff is in delete list.
					bool inDelete = false;
					for(int ii = 0; ii < *listCursor; ++ii) {
						if(strcmp(staffArr[i].id, deleteList[ii]) == 0) {
							inDelete = true;
							break;
						}
					}
					if(!inDelete) {
						++acc;
					}
				}
			}
			strcpy(deleteList[*listCursor], staffArr[i-1].id);
		}

		++*listCursor;
	}
	retval = *listCursor;
	
	// Modify staff's $passHash to zero.
	int acc = 0;
	for(int i = 0; i < len; ++i) {
		bool match = false;
		for(int ii = 0; ii < *listCursor; ++ii) {
			if(!isStaffDeleted(staffArr[i]) != 0 && strcmp(staffArr[i].id, deleteListData[ii]) == 0) {
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

	if(staffFile != NULL && fclose(staffFile) == EOF) {
		perror("Error (Closing staff file, file data might not be saved.) ");
		pause();
		retval = 1;
	}
	return retval;
}


int promptStaffDetails(char* buf, enum StaffModifiableFields selection) {
	char* promptMessage;
	int maxLen = STAFF_BUF_MAX;
	bool valid = false;

	bool isFlipped = false;
	selection = (selection&(1<<(sizeof(selection)*8-1))) != 0 ? isFlipped=true, ~selection : selection;

	// All uses the same length as their buffer size to check for user input overflow.
	switch(selection) {
		case SE_ID:
			promptMessage = "ID (S0001)";
			maxLen = 6;
			break;
		case SE_NAME:
			promptMessage = "Name (John Smith)";
			maxLen = 128;
			break;
		case SE_POSITION:
			promptMessage = "Position (Admin)";
			maxLen = 32;
			break;
		case SE_PHONE:
			promptMessage = "Phone (0123456789)";
			maxLen = 16;
			break;
		case SE_IC:
			promptMessage = "IC (000101070000)";
			maxLen = 15;
			break;
		default:
			printf("Error (Unimplemented)\n");
			return -15;
	};

	while(!valid) {
		valid = true;
		if(!isFlipped) {
			printf("Staff's %s: ", promptMessage);
		}

		// Scan for input. Also discards any leading spaces.
		int i = 0;
		int c = 0;
		while((c = getchar(), c != '\r' && c != '\n') && i < maxLen) {
			if(i == 0 && c == ' ') {
				continue;
			}
			if(c == EOF) {
				return EOF;
			}

			buf[i++] = c;
		}
		if(c != '\n') {
			truncate();
		}

		if(i == maxLen) {
			printf("Please enter a shorter value!\n\n");
			valid = false;
		}
		// Remove trailing spaces.
		while(buf[--i] == ' ');
		buf[i+1] = 0;

		// Control characters detection.
		if(buf[0] == ':') {
			if(toupper(buf[1]) == 'Q' || toupper(buf[1]) == 'W') {
				return -2;
			} else if(toupper(buf[1]) == 'H') {
				printf(
					"HELP: Enter a value according to what is prompted.\n"
					"      It shows the prompted item, and an example of it enclosed in brackets.\n\n"
				);
				return promptStaffDetails(buf, selection^~(isFlipped-1));
			}
		}

		if(buf[0] == 0) {
			printf("Please enter a value!\n\n");
			valid = false;
		}

		if(valid) {
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
					if(buf[0] != '0') {
						printf("Please enter a valid phone number!\n\n");
						valid = false;
					}

					int phoneLen = strlen(buf);
					int numbersLen = 0;

					for(; buf[i] && valid; ++i) {
						if(buf[i] == '-' || buf[i] == ' ') {
							// Remove hyphens or space for the user.
							memmove(buf+i, buf+i+1, phoneLen-numbersLen); // Moves the null character too.
							--phoneLen;
							--i; // Stay on the same index.
						} else if(buf[i] < '0' || buf[i] > '9') {
							printf("The phone number should only contain numbers!\n\n");
							valid = false;
							break;
						} else {
							++numbersLen;
						}
					}

					// All are numbers, but invalid length.
					if(valid && (numbersLen < 9 || numbersLen > 12)) {
						printf("Please enter a valid phone number!\n\n");
						valid = false;
					}
					break;
				}
				case SE_IC: {
					// Check if it's all numbers.
					int icLen = strlen(buf);
					int i = 0;
					int numbersLen = 0;
					for(i = 0; i < buf[i]; ++i) {
						if(buf[i] == '-' || buf[i] == ' ') {
							// Same method as phone number.
							// O(n^2) but should be fast enough for n < 16.
							memmove(buf+i, buf+i+1, icLen-numbersLen);
							--icLen;
							--i; // Stay on the same index.
						} else if(buf[i] < '0' || buf[i] > '9') {
							printf("IC number should only contain numbers!\n\n");
							valid = false;
							break;
						} else {
							++numbersLen;
						}
					}
					if(valid && i != 12) {
						printf("Please enter IC number with the correct length!\n\n");
						valid = false;
					}

					if(valid) {
						// Extract year.
						char c = buf[2];
						buf[2] = 0;
						short year = atoi(buf);
						buf[2] = c;

						// Extract month.
						c = buf[4];
						buf[4] = 0;
						short month = atoi(buf+2);
						buf[4] = c;

						// Extract day.
						c = buf[6];
						buf[6] = 0;
						short day = atoi(buf+4);
						buf[6] = c;

						// Extract state code.
						c = buf[8];
						buf[8] = 0;
						short state = atoi(buf+6);
						buf[8] = c;

						// NOTE: Year in IC is not validated since humans can live past 99 years old lol.

						// Validate month.
						if(valid && (month < 1 || month > 12)) {
							printf("Please enter a valid month!\n\n");
							valid = false;
						}

						// Validate day.
						if(
							valid &&
							(
								day < 1 || // Day should not be less than 1.
								(
									month != 2 && (
										(buf[3]+(buf[3] >= '8' || buf[2] == '1'))%2 == 1 ? // Months that have 31 days.
											day > 31 :
											day > 30
									)
								) ||
								(
									month == 2 && (
										(
											(year%100 == 0 && year%400 != 0) ? // End of century years that is not a leap year.
											day > 28 :
											day > 29
										) ||
										year%4 == 0 ? day > 29 : day > 28
									)
								)
							)
						) {
							printf("Please enter a valid day of month!\n\n");
							valid = false;
						}

						if(valid && (state == 0 || (state >= 17 && state <= 20) || state == 69 || state == 70 || state == 73 || state == 80 || state == 81 || (state >= 94 && state <= 97))) {
							printf("Please enter a valid state number!\n\n");
							valid = false;
						}
					}
					break;
				}
				default:;
					// Do nothing.
			}
		}
		// Hands back control to callee if enum is passed in with its bits flipped.
		if(isFlipped && !valid) {
			return -14;
		}
	}

	// No error.
	return 0;
}


DisplayStaffOptions displayStaffOptionsInit(void) {
	return (DisplayStaffOptions) {
		NULL,
		NULL,
		{ 0, 0, 0, 0, 0 },
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


int displaySelectedStaff(DisplayStaffOptions* options) {
	int retval = 0;
	FILE* staffFile = fopen("staff.bin", "rb");
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
		// Designing a backup solution really will do die me ah.
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
		pause();
		retval = -4;
		goto CLEANUP;
	}

	// Traverse the file and mark ith bit of $includeFlag as 0/1 to determine if it should be included/excluded from print.
	int total = 0; // Total number of matches (Will be printed).

	for(int i = 0; i < options->metadata.totalEntries; ++i) {
		if(isStaffDeleted(staffArr[i]) ? !options->displayDeleted : !options->displayExisting) {
			// Hide staff.
			includeFlag[i/8] &= ~(1 << (i%8));
			continue;
		}

		for(int ii = 0; ii < options->idListLen; ++ii) {
			if(strcmp(staffArr[i].id, options->idList[ii]) == 0) {
				// If exclude mode, set bit as 0.
				// If include mode, set bit as 1.
				if(!options->isInclude) {
					includeFlag[i/8] &= ~(1 << (i%8));
				} else {
					includeFlag[i/8] |= (1 << (i%8));
				}
				break; // Marked as include, don't need to check against other IDs.
			}
		}
	}

	for(int i = 0; i < options->metadata.totalEntries; ++i) {
		if((includeFlag[i/8]&(1<<(i%8))) != 0) {
			++total;
		}
	}
	options->metadata.matchedLength = total;

	// Handle page out of bounds.
	if(options->page < 0) {
		options->page = 0;
	} else if(options->page * options->entriesPerPage >= total) {
		options->page = options->metadata.matchedLength/(options->entriesPerPage+1);
	}

	// Stores each starting position of the page. Indexed with $curPage.
	arrCursorHist = calloc(1, sizeof(int) * ((options->metadata.totalEntries+(options->entriesPerPage-1))/options->entriesPerPage));
	if(arrCursorHist == NULL) {
		perror("Error (malloc $arrCursorHist)");
		pause();
		retval = -4;
		goto CLEANUP;
	}
	arrCursorHist[0] = 0;

	// Fill in each starting index of match from staff file to each page.
	for(int i = 0, acc = 0, curPage = 1; i < options->metadata.totalEntries; ++i) {
		if((includeFlag[i/8] & (1<<(i%8))) != 0) {
			if(++acc > options->entriesPerPage) {
				arrCursorHist[curPage] = i;
				acc = 1; // Already included staffArr[i].

				++curPage;
			}
		}
	}

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
			4, 30,
			8, 15,
			5, 13,
			2, 14
		};

		int cols = 0;
		// Print headers.
		printf("Number    "); // Column of the number of current row.
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
					// Allocate a width of 30 cols for name.
					// If a name exceeds 27 characters (excluding null), ellipsis will be added.
					printf("%-30s", "NAME");
					break;
				case SE_POSITION:
					printf("%-15s", "POSITION");
					break;
				case SE_PHONE:
					// <16 because of max valid phone length.
					// Two additional characters to prettify print.
					printf("%-13s", "PHONE");
					break;
				case SE_IC:
					printf("%-14s", "IC");
				default:;
					// Do nothing.
			}
			printf("    ");
		}
		if(options->displayDeleted) {
			printf("DELETED");
		}
		putchar('\n');

		#ifdef __linux__
		// Straight from DOS but Windows still doesn't support it.
		#define printDiv(n) do{ for(int _i = (n); _i; _i--) printf("─"); } while(0)
		#else
		#define printDiv(n) do{ for(int _i = (n); _i; _i--) putchar('-'); } while(0)
		#endif

		// Print header and content divider.
		printDiv(6); // Divider for number column.
		printf("    ");
		for(int i = 0; i < STAFF_ENUM_LENGTH*2; i += 2) {
			if(!options->displayList[i/2]) {
				continue;
			}

			printDiv(printWidth[i]);

			// Plus 4 for row divider.
			for(int ii = printWidth[i]; ii < printWidth[i+1]+4; ++ii) {
				putchar(' ');
			}
		}
		if(options->displayDeleted) {
			printDiv(7);
		}
		putchar('\n');

		// Print staff details from staffArray.
		int arrCursor = arrCursorHist[options->page];
		int read = options->page * options->entriesPerPage;

		// If this is the last page, read until $total, else read until before the starting index of another page.
		for(; (total-(options->page*options->entriesPerPage) <= options->entriesPerPage) ? (read < total) : (arrCursor < arrCursorHist[options->page+1]); ++arrCursor) {
			if((includeFlag[arrCursor/8]&(1<<(arrCursor%8))) == 0) {
				continue;
			}

			if(read != options->page*options->entriesPerPage) {
				// Print newline first then staff data.
				putchar('\n');
			}

			// Print column number.
			printf("%6d    ", ++read);


			for(int i = 0; i < STAFF_ENUM_LENGTH; ++i) {
				if(!options->displayList[i]) {
					continue;
				}
				switch(i) {
					case SE_ID:
						printf("%s   ", staffArr[arrCursor].id);
						break;
					case SE_NAME:
						if(strlen(staffArr[arrCursor].details.name) > 28) {
							int c = staffArr[arrCursor].details.name[28];
							staffArr[arrCursor].details.name[28] = 0;
							printf("%s..", staffArr[arrCursor].details.name);
							staffArr[arrCursor].details.name[28] = c;
						} else {
							printf("%-28s  ", staffArr[arrCursor].details.name);
						}
						break;
					case SE_POSITION:
						if(strlen(staffArr[arrCursor].details.position) > 13) {
							int c = staffArr[arrCursor].details.position[13];
							staffArr[arrCursor].details.position[13] = 0;
							printf("%s..", staffArr[arrCursor].details.position);
							staffArr[arrCursor].details.position[13] = c;
						} else {
							printf("%-13s  ", staffArr[arrCursor].details.position);
						}
						break;
					case SE_PHONE:
						if(staffArr[arrCursor].details.phone[1] != '1') {
							putchar('0');
							putchar(staffArr[arrCursor].details.phone[1]);
							putchar(' ');
							putchar(' ');

							int remaining = strlen(staffArr[arrCursor].details.phone+2);
							for(int i = 2; i < 2+remaining/2; ++i) {
								putchar(staffArr[arrCursor].details.phone[i]);
							}
							if(remaining != 8) {
								putchar(' ');
							}
							printf(" %-4s", staffArr[arrCursor].details.phone+2+remaining/2);
						} else {
							putchar('0');
							putchar(staffArr[arrCursor].details.phone[1]);
							putchar(staffArr[arrCursor].details.phone[2]);
							putchar(' ');

							int remaining = strlen(staffArr[arrCursor].details.phone+3);
							for(int i = 3; i < 3+remaining/2; ++i) {
								putchar(staffArr[arrCursor].details.phone[i]);
							}
							if(remaining != 8) {
								putchar(' ');
							}
							printf(" %-4s", staffArr[arrCursor].details.phone+3+remaining/2);
						}
						break;
					case SE_IC:
						for(int i = 0; i < 6; ++i) {
							printf("%c", staffArr[arrCursor].details.ic[i]);
						}
						putchar('-');
						for(int i = 6; i < 8; ++i) {
							printf("%c", staffArr[arrCursor].details.ic[i]);
						}
						putchar('-');
						for(int i = 8; i < 12; ++i) {
							printf("%c", staffArr[arrCursor].details.ic[i]);
						}
					default:;
						// Do nothing.
				}
				printf("    ");
			}

			if(options->displayDeleted) {
				if(isStaffDeleted(staffArr[arrCursor])) {
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

		if(total == 0) {
			printf("  No matching entries!\n");
		}

		// Print table border
		for(int i = 1; i < STAFF_ENUM_LENGTH*2; i += 2) {
			if(!options->displayList[i/2]) {
				continue;
			}
			for(int ii = printWidth[i]+4*(i+1 != STAFF_ENUM_LENGTH*2 || options->displayDeleted); ii; --ii) {
				printDiv(1);
			}
		}
		printDiv(10); // Number column.
		if(options->displayDeleted) {
			printDiv(10);
		}
		putchar('\n');

		if(total <= 1) {
			printf("Displaying %d entry", read);
		} else {
			printf("Displaying %d entries", read);
		}

		if(options->metadata.totalEntries >= 0) {
			printf(" of %d entr%s.", total, total < 2 ? "y" : "ies");
		}
		printf(" (Page %d)\n", options->page+1);

		if(options->isInteractive) {
			// To keep the system consistent, precedes with a colon. (Optional now).
			printf("\nEnter 'n' for next page, 'b' to go back a page, 'q' to quit: ");
			char action[3];

			if(scanf("%2s", action) == EOF) {
				retval = EOF;
				goto CLEANUP;
			}
			truncate();
			if(action[0] == ':') {
				action[0] = action[1];
			}

			if(toupper(action[0]) == 'Q') {
				break;
			} else if(toupper(action[0]) == 'B' && options->page > 0) {
				--options->page;
			} else if(toupper(action[0]) == 'N' && read < total) {
				++options->page;
			} else if(toupper(action[0]) == 'H') {
				cls();
				printf(
					"HELP\n"
					"====\n"
					"  Uses 'n' or 'b' to turn to the next page or go back a page.\n"
					"  Enter 'q' to quit interactive mode.\n"
					"  Actions:\n"
					"    n (Next page.)\n"
					"    b (Go back a page.)\n"
					"    q (Quit.)\n\n"
				);
				pause();
			}
			cls();
		} else {
			putchar('\n');
			break;
		}
	}
	retval = total;

CLEANUP:
	#undef printDiv
	free(staffArr);
	free(includeFlag);
	free(arrCursorHist);
	if(staffFile != NULL) {
		fclose(staffFile);
	}
	return retval;
}


Staff loginStaff(void) {
	Staff s;
	char buf[STAFF_BUF_MAX];
	FILE* staffFile = fopen("staff.bin", "rb");

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		// Do not pause(). The file may not been initialised and it is being done silently.
		s.id[0] = 0;
		s.id[1] = -3;
		goto CLEANUP;
	}

	while(1) {
		printf(
			"LOGIN\n"
			"=====\n"
		);
		int res = promptStaffDetails(buf, SE_ID);
		if(res == EOF) {
			s.id[0] = 0;
			s.id[1] = EOF;
			goto CLEANUP;
		}

		// Do not allow abort.
		if(res == -2) {
			continue;
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
					s.id[0] = 0;
					s.id[1] = EOF;
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
	if(staffFile != NULL) {
		fclose(staffFile);
	}
	return s;
}


int menuStaff(Staff* loggedInUser) {
	while(1) {
		cls();
		char action[3];
		printf(
			"STAFF MODULE\n"
			"============\n"
			"Welcome %s!\n\n"
			"- Add\n"
			"- Display\n"
			"- Delete\n"
			"- Modify\n"
			"- Search\n"
			"- Report\n\n"
			"- Quit menu\n\n"
			"Enter a function: ",
			loggedInUser->details.name
		);

		if(scanf("%2s", action) == EOF) {
			return EOF;
		}
		truncate();
		if(action[0] == ':') {
			action[0] = action[1];
		}

		switch(toupper(*action)) {
			case 'A':
				addStaff();
				break;
			case 'D':
				if(action[1] == '\0') {
					printf("\nPlease enter one more character (DI[SPLAY] / DE[LETE]) to distinguish between 2 similar functions!\n");
					pause();
				} else if(toupper(action[1]) == 'I') {
					displayStaff();
				} else if(toupper(action[1]) == 'E') {
					deleteStaff();
				} else {
					printf("Please enter an existing function!\n");
					pause();
				}
				break;
			case 'M':
				modifyStaff();
				break;
			case 'S':
				searchStaff();
				break;
			case 'R':
				reportStaff();
				break;
			case 'Q':
				return 0;
				// No break here because of return.
			case 'H':
				cls();
				printf(
					"HELP\n"
					"====\n"
					"  Choose one of the function of the staff module by typing the first few characters of the corresponding function.\n"
					"  Action:\n"
					"    :h (Help.)\n"
					"    :q (Quit.)\n"
					"    A  (Add staff page.)\n"
					"  Examples:\n"
					"    A\n"
					"    (Goes to add staff page.)\n"
					"    Se\n"
					"    (Goes to search staff page.)\n\n"
					"  (Congrats on finding this unindexed page!)\n\n"
				);
				pause();
				break;
			default:
				printf("Please enter an existing function!\n");
				pause();
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
	const short SIGMA[12][16] = {
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
		const short* S = SIGMA[i];

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


int KMPSearch(char* text, char* query, bool ignoreCase) {
	int LPS[STAFF_BUF_MAX] = { 0 };
	int textLen = strlen(text);
	int queryLen = strlen(query);
	if(textLen == 0 || queryLen == 0) {
		return 0;
	}
	int* lookup = LPS+1; // Add a '-1' index.

	// Build array.
	for(int r = 1, l = 0; r < queryLen; ++r) {
		if(ignoreCase ? toupper(query[r]) == toupper(query[l]) : query[r] == query[l]) {
			lookup[r] = ++l;
		} else {
			l = 0;
		}
	}

	int qIdx = 0;
	int i = 0;
	while(i < textLen) {
		if(ignoreCase ? toupper(query[qIdx]) == toupper(text[i]) : query[qIdx] == text[i]) {
			++qIdx;
			++i;
		} else {
			qIdx = lookup[qIdx-1];
			if(qIdx == 0) {
				++i;
			}
		}

		if(qIdx == queryLen) {
			return i-queryLen;
		}
	}

	return -1;
}


bool LIKE(char* text, char* query, bool ignoreCase) {
	int textLen = strlen(text);
	int queryLen = strlen(query);

	int wildCardLastIdx = -1;
	for(int i = 0; i < queryLen; ++i) {
		if(query[i] == '%') {
			wildCardLastIdx = i;
		}
	}

	int qIdx = 0;			// Current index on the query string.
	int textOffset = 0;		// Or rather characters matched.
	int lastWildcard = -1;	// Last index of '%', to locate the start of a '%' wildcard group.
	bool match = true;

	// Compare the trivial part of the query.
	for(; qIdx < queryLen && query[qIdx] != '%'; ++qIdx) {
		if(query[qIdx] == '_' || (ignoreCase ? toupper(query[qIdx]) == toupper(text[textOffset]) : query[qIdx] == text[textOffset])) {
			++textOffset;
			if(textOffset > textLen) {
				match = false;
				break;
			}
		} else {
			match = false;
			break;
		}
	}

	int substringEnd = -1; // The index in a % wildcard group that marks the first occurence of _.
	for(; match && qIdx <= wildCardLastIdx; ++qIdx) {
		if(query[qIdx] == '%') {
			// Don't search if there's no wildcard initialised and don't search 
			if(lastWildcard != -1 && lastWildcard+1 != qIdx) {
				int offset = 0;
				// Set null character temporarily.
				query[substringEnd == -1 ? qIdx : substringEnd] = 0;

				// A do while loop that will keep looping if a match is not found until offset == -1.
				do {
					offset = KMPSearch(text+textOffset, query+lastWildcard+1, ignoreCase);

					if(substringEnd != -1) {
						if(offset == -1) {
							break;
						}

						int localTextOffset = textOffset+offset+1;
						int localIdx = substringEnd+1;
						// Not checking for overflow since if this current one overflows then no match is left.
						bool localMatch = true;
						for(; localIdx < qIdx; ++localIdx) {
							if(query[localIdx] == '_') {
								++localTextOffset;
							} else if(ignoreCase ? toupper(query[localIdx]) == toupper(text[localTextOffset]) : query[localIdx] == text[localTextOffset]) {
								++localTextOffset;
								++localIdx;
							} else {
								localMatch = false;
								break;
							}
						}
						if(!localMatch) {
							// O(n^2) search, pain 85.
							++textOffset;
							continue;
						}

					}
				} while(0);
				query[substringEnd == -1 ? qIdx : substringEnd] = substringEnd == -1 ? '%' : '_';

				if(offset == -1) {
					match = false;
					break;
				}

				textOffset += offset+qIdx-lastWildcard-1;
			}

			// Perform string matching for the string suffix.
			if(qIdx == wildCardLastIdx) {
				if(textLen-textOffset < queryLen-wildCardLastIdx-1 && queryLen-wildCardLastIdx != 1) {
					match = false;
					break;
				}
				for(int r = 1; r <= queryLen-qIdx-1; ++r) {
					if(query[queryLen-r] != '_' && (ignoreCase ? toupper(query[queryLen-r]) != toupper(text[textLen-r]) : query[queryLen-r] != text[textLen-r])) {
						match = false;
						break;
					}
				}
				// Matched the whole string.
				textOffset = textLen;
			}
			lastWildcard = qIdx;
			substringEnd = -1;
		} else if(query[qIdx] == '_' && substringEnd == -1) {
			substringEnd = qIdx;
		}

		if(textOffset > textLen) {
			match = false;
			break;
		}
	}

	if(textOffset < textLen && query[queryLen-1] != '%') {
		match = false;
	}

	return match;
}

void menuMember() {};
void menuFacility() {};
void menuBooking() {};
void menuUsage() {};

int main(void) {
	Staff loggedInUser;
	bool loggedIn = false;

	// while(1) {
	// char buf[30];
	// char buf2[30];
	// printf("text: ");
	// scanf("%[^\n]", buf);
	// truncate();
	// printf("query: ");
	// scanf("%[^\n]", buf2);
	// truncate();
	// printf("res: %d\n", LIKE(buf, buf2, true));
	// }
	printf("logo\n");
	do {
		char choice[3];
		if(loggedIn) {
			// Excluding staff information.
			char* modules[] = { "Member", "Facility" , "FacilityUsage", "Booking", "Exit" };
			int n = 0;

			printf("\n\n");
			if(KMPSearch(loggedInUser.details.position, "ADMIN", true) == 0) {
				printf("%d. Staff Information \n", ++n);
			}
			for(int i = 0; i < (int) (sizeof(modules)/sizeof(char*)); ++i) {
				printf("%d. %s \n", ++n, modules[i]);
			}
			printf("Enter choice > ");
		} else {
			bool first = true; // To avoid infinite loop.

PROMPT_LOGIN:
			loggedInUser = loginStaff();
				
			if(loggedInUser.id[0] != 0) {
				loggedIn = true;
				continue;
			} else if(loggedInUser.id[1] == EOF) {
				goto EOF_EXIT;
			} else if(loggedInUser.id[1] == -3) {
				if(first) {
					// Staff file not found. Insert a file with default value.
					FILE* staffFile = fopen("staff.bin", "wb");
					if(staffFile != NULL) {
						fwrite(&(Staff) { "S0000", { "ADMIN", "Admin", "0123456789", "000101010000" }, computeHash("ADMIN") }, sizeof(Staff), 1, staffFile);
						fclose(staffFile);
					}
					first = false;
					goto PROMPT_LOGIN; // Try to return back to loginStaff() again with the init-ed file, saves the user a step.
				} else {
					printf(
						"If you see this message, it is time to get a new hard drive...\n"
						"(or the program is in a directory that does not allow files to be created. ¯\\_(ツ)_/¯)\n\n"
						"Exiting...\n"
					);
					return 0;
				}
			}
		}

		if(scanf("%2s", choice) == EOF) {
			goto EOF_EXIT;
		}
		truncate();

		bool valid = true;
		for(int i = 0; choice[i]; ++i) {
			if(choice[i] < '0' || choice[i] > '9') {
				valid = false;
				break;
			}
		}

		if(!valid) {
			printf("Please enter a valid number! \n");
			continue;
		}
		if(atoi(choice) == 0) {
			// To avoid unauthorised user enter staff module using 0.
			printf("Invalid choice! \n");
			continue;
		}

		switch(atoi(choice) + (KMPSearch(loggedInUser.details.position, "ADMIN", true) != 0)) {
			case 1: {
				menuStaff(&loggedInUser);
				break;
			}
			case 2: {
				menuMember();
				break;
			}
			case 3: {
				menuFacility();
				break;
			}
			case 4: {
				menuUsage();
				break;
			}
			case 5: {
				menuBooking();
				break;
			}
			case 6: {
				printf("thank you \n");
				pause();
				return 0;
				// No break because of return.
			}
			default: {
				printf("Invalid choice! \n");
			}
		}
	} while(1);

	return 0;
EOF_EXIT:
	printf("EOF signal received, exiting program.\n");
	return 0;
}

#undef STAFF_ENUM_LENGTH
#undef STAFF_BUF_MAX
#undef ENTRIES_PER_PAGE
#undef isStaffDeleted
#undef truncate
#undef pause
#undef ENABLE_CLS
#undef cls
