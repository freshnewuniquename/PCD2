#include<ctype.h>	// toupper()
#include<stdbool.h>	// bool, false, true
#include<stdio.h>	// fread(), fwrite(), printf(), scanf(), rewind(), getchar(), fseek(), ftell(), perror(), stdin, EOF, FILE, SEEK_END
#include<stdlib.h>	// exit(), atoi(), malloc(), free()
#include<string.h>	// strcpy(), strlen(), strcmp()


typedef unsigned long long u64;

/*
	Data dictionary
	===============
	Name			Example				Description
	----			------				-----------
	ID				S000001				An unique ID assigned to the staff.
	name			John Smith	 		Stores the name of the staff.
	position		Administrator		A string that records the position of the staff, has no effect on what they can access.
	passwordHash	<64 bits binary>	A read-only hash of the staff's password.
	permission		<32 bits binary>	A binary flag to determine which function the staff has access to. Go to $ACCESS for more.

	NOTE: Modify $MAX as well if the largest buffer size is changed (current max, $name, is 128 bytes).
*/
typedef struct {
	char id[8];
	struct {
		char name[128];
		char position[64];
		char phone[16];
	} details;
	u64 passHash;
	int permission;
	// 4 bytes padding here.
} Staff;

/*
	TODO: What is shown on menu should be based on this.
	ACCESS
	======
	If the nth bit of $Staff.permission is set, the corresponding staff will be able to access the function assigned.
		0th bit		addStaff()
		1st bit		searchStaff()
		2nd bit		modifyStaff()
		...
*/
const char* ACCESS[] = {
	"addStaff",
	"searchStaff",
	"modifyStaff",
	"displayStaff",
	"reportStaff",
	"deleteStaff"
};


/*
	This enum list all the modifable fields found in the Staff{}.
	This enum will be used in prompt() to determine which field is going to be modified.
	
	NOTE: The permission field must take the current logged in staff's permission field in consideration, and should be less than or equal to the current logged in staff.
		  For more information, see the documentation available in the promptStaffDetails() function.
*/
enum StaffModifiableFields { SE_ID, SE_NAME, SE_POSITION, SE_PERMISSION };
const int STAFF_ENUM_LENGTH = sizeof(enum StaffModifiableFields)/sizeof(SE_ID);


// Define maximum char array size needed for Staff struct elements buffer.
#define MAX 128

// Define how many rows of records to display in displayStaff().
#define ENTRIES_PER_PAGE 15

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
	} while(0)


// ----- START OF HEADERS -----
/**
 * @brief	Presents a screen to add more staff to the staff record.
 * 
 * @param	s	Staff struct of the logged in staff. Passed by pointer to reduce stack usage.
 *
 * @retval	0	Staff successfully added.
 * @retval	1	Staff failed to be added (File operation error).
 * @retval	2	Staff failed to be added (Current staff has insufficient permission).
 * @retval	3	Staff addition cancelled (Add operation was cancelled by user).
 * @retval	EOF	Staff addition cancelled (EOF signal received).
 */
int staffAdd(Staff* s);


/**
 * @brief	Presents a screen to add more staff to the staff record.
 * 
 * @param	s	Staff struct of the logged in staff. Passed by pointer to reduce stack usage.
 *
 * @retval	0	Staff successfully added.
 * @retval	1	Staff failed to be added (Staff file cannot be opened).
 * @retval	2	Staff failed to be added (Current staff has insufficient permission).
 * @retval	3	Staff failed to be added (Add operation was cancelled by user).
 */
int staffSearch(Staff* s);


int staffModify(Staff* s);


int staffDisplay(Staff* s);


int staffReport(Staff* s);


int staffDelete(Staff* s);


/**
 * @brief	Prompts the user to enter selected staff details and validate them.
 *
 * This function handles the user input by validating them for correctness.
 * The inputted data will be truncated to the appropriate size to avoid buffer overflow.
 * This function makes the assumption that $buf is appropriately sized (>= to the size of corresponding field in Staff{}).
 * Any data truncated will not be notified and is up to the callee to show the user what was left of their input.
 * If EOF is received during the prompt, $buf is left in an incomplete state. (May contain invalid data)
 *
 * XXX: HACK:	An int* array pointer will be passed in through $buf if $selection is PERMISSION.
 * 				E.g.
 *
 * 				    int curPerm = 0;
 * 				    int newPerm = curPerm;
 *
 * 				    promptStaffDetails([&curPerm, &newPerm], PERMISSION);
 *
 * @param	buf	A pointer to the appropriately sized buffer.
 *
 * @retval	EOF	EOF signal received, aborting prompt.
 * @retval	0	Read and validated successfully.
 * @retval	1	Unimplemented.
 */
int staffPromptDetails(char* buf, enum StaffModifiableFields selection);


/**
 * @brief	Display the passed staff information in a nice table format.
 * 
 * @param	idList			A string array containing the list of staff to include/exclude in printing.
 * @param	idListLen		An integer that specify the length of $idList. If $idListLen is negative, number of staff to exclude is the unary of $idListLen (print staffs not included in $idList).
 * @param	printList		A enum list to print only the fields provided according to the order passed in.
 * @param	printListLen	Length of enum list.
 *
 * @retval	0	Print operation successful.
 * @retval	1	Print operation failed, file operation error.
 * @retval	2	Print operation failed, malloc() failed.
 */
int staffDisplaySelected(char** idList, int idListLen, enum StaffModifiableFields* printList, int printListLen);


/**
 * @brief	Presents the user with a login screen and prompts the user to login.
 *
 * A login screen that allows staffs to login.
 * The staff's ID will be used to determine which staff is planning to login.
 * If the staff's ID is valid and matched one of the records, a password will be prompted.
 * If the inputted password is incorrect thrice, staff ID will be prompted again in case the staff ID was entered incorrectly.
 * A complete Staff struct with the corresponding staff details will be returned, after the staff has logged in.
 *
 * @return	First byte of struct is filled with -1 to indicate error.
 * @return	Logged in user's matching Staff struct.
 */
Staff staffLogin(void);


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
 * @brief	Compress function defined by BLAKE2b.
 *
 * @param	hash		Pointer to the 64 bytes hash array.
 * @param	msg			Pointer to one of the 128 bytes chunk of the inputted message.
 * @param	compressed	Number of bytes that has been compressed (including $msg).
 * @param	isLastBlock	A boolean value that tells whether or not the current message chunk is the last one.
 */
void BLAKE2bF(u64* hash, char* msg, u64 compressed, bool isLastBlock);


/**
 * @brief	Hashes the inputted string (password) to a one-way BLAKE2 hash.
 *
 * A BLAKE2 message digest hash function that takes in a message to produce a fixed sized hash.
 * This implementation of the BLAKE2 hash is in accordance with the BLAKE2b algorithm, designed for 64 bits system.
 * This implementation does not include keys, nor salting the input.
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
// ----- END OF HEADERS -----


int staffAdd(Staff* s) {
	Staff newStaff;
	FILE* staffFile = fopen("staff.bin", "ab+");

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		return 1;
	}

	if((s->permission & 0x1) == 0) {
		printf("Insufficient permission!\n");
		return 2;
	}

	char buf[MAX];

	// Prompt ID
	while(1) {
		if(promptStaffDetails(buf, ID) == EOF) {
			return EOF;
		}

		Staff tmp;
		bool exists = false;

		rewind(staffFile);
		while(fread(&tmp, sizeof(Staff), 1, staffFile) == 1) {
			if(strcmp(tmp.id, buf) == 0) {
				exists = true;
			}
		}

		if(exists) {
			printf("Staff with the same ID exixts!\n\n");
		} else {
			strcpy(newStaff.id, buf);
			break;
		}
	}

	// Prompt name
	// TODO: tell user what was received since scanf auto truncates.
	if(staffPromptDetails(buf, NAME) == EOF) {
		return EOF;
	}
	truncate();
	strcpy(newStaff.details.name, buf);
	

	// Prompt position
	// TODO: same as name
	if(staffPromptDetails(buf, POSITION) == EOF) {
		return EOF;
	}
	truncate();
	strcpy(newStaff.details.position, buf);
	

	// Prompt password
	while(1) {
		// TODO: tell the user the limit is 127 chars.
		// NOTE: Implemented independantly because password hash should not be modifiable.
		printf("Password (E.g. Ky54asdf'_. dQw4w9WgXcQ): ");
		
		if(scanf("%127[^\n]", buf) == EOF) {
			return EOF;
		}
		truncate();

		u64 hash1 = computeHash(buf);

		printf("Re-enter password: ");

		if(scanf("%127[^\n]", buf) == EOF) {
			return EOF;
		}
		truncate();

		if(hash1 == computeHash(buf)) {
			newStaff.passHash = hash1;
			break;
		} else {
			printf("Passwords do not match!\n\n");
		}
	}

	newStaff.permission = s->permission;

	while(1) {
		// TODO: wats tis
	}

	if(fwrite(&newStaff, sizeof(newStaff), 1, staffFile) == 0) {
		printf("An error occured while writing to file buffer!\n");
		return 1;
	}

	if(fclose(staffFile) == EOF) {
		perror("Error (Closing staff file) ");
		return 1;
	} else {
		printf("New staff details saved successfully!\n\n");
	}

	return 0;
}


int staffSearch(Staff* s) {
	// TODO: implement
	return 0;
}


int staffModify(Staff* s) {
	FILE* staffFile = fopen("staff.bin", "rb+");
	
	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		return 1;
	}

	if(s->permission & 0x4) {
		printf("Insufficient permission!\n");
		return 2;
	}

	Staff chosenStaff;
	char buf[MAX];
	bool found = false;

	int numModified = 0;
	while(1) {
		printf("Type a staff ID to modify their staff details or q to quit.");
		if(promptStaffDetails(buf, ID) == EOF) {
			return EOF;
		}
		truncate();

		// Search for matching records.
		found = false;

		while(fread(&s, sizeof(Staff), 1, staffFile) != 0) {
			if(strcmp(chosenStaff.id, buf) == 0) {
				found = true;
				break;
			}
		}
		
		if(found) {
			// Break out of search loop.
			break;
		} else {
			printf("Staff ID entered does not match any records!\n");
			pause();
		}
	}


	if(fclose(staffFile) != 0) {
		perror("Error (Closing staff file)");
		return 1;
	}
	return 0;
}


int staffDisplay(Staff* s) {
	FILE* staffFile = fopen("staff.bin", "rb");

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		return 1;
	}

	if((s->permission & 0x8) == 0) {
		printf("Insufficient permission!\n");
		return 2;
	}

	// Get file size.
	int res = fseek(staffFile, 0, SEEK_END);
	int len = -1;
	if(res == 0) {
		// If fseek is successful, get file size.
		len = ftell(staffFile);
		len /= sizeof(Staff);
		rewind(staffFile);
	} else {
		perror("Error (fseek to end)");
	}

	// Allocate memory to hold a page of record.
	Staff* page = (Staff*) malloc(sizeof(Staff)*ENTRIES_PER_PAGE);
	if(page == NULL) {
		perror("Error (malloc)");
	}

	int curPage = 1;

	while(1) {
		printf(
			"DISPLAY\n"
			"-------\n"
		);
	}

	free(page);
	fclose(staffFile);

	return 0;
}


int staffDelete(Staff s) {
	FILE* staffFile = fopen("staff.bin", "r+");
	// Will only be used if malloc() failed.
	FILE* staffFileTmp;
	
	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		return 1;
	}


	int size = -1;
	if(fseek(staff, 0, SEEK_END) != 0) {
		// fseek failed.
		perror("Error (fseek failed) ");
	} else {
		size = ftell(staffFile);
		rewind(staffFile);
	}

	Staff* staffArr;
	int arrCapacity = len;
	if(len == -1) {
		// fseek failed. Set a default size for malloc().
		// Allocate memory that can fit 1024 entries of Staff{}. (around 200KB)
		arrCapacity = 1024;
	}
	// Try to load all of the Staff file data to memory so that a tmp file doesn't have to be created.
	staffArr = (Staff*) malloc(arrCapacity*sizeof(Staff));

	if(staffArr == NULL) {
		// malloc() failed.

	} else if(len != -1) {
		// malloc() suceeded and fseek suceeded.
		fread(staffArr, 1, size, staffFile);
	} else {
		// malloc() suceeded but fseek failed.
		// Will have to extend malloc when needed.
		int i = 0;
		while(fread(staffArr+i, sizeof(Staff), 1, tmp) == 1) {
			if(++i > 1024) {
				// TODO: Apply fix for realloc() fail.
				arrCapacity *= 2;
				staffArr = realloc(staffArr, arrCapacity);
			}
		}
	}

	
	free(staffArr);

	if(fclose(staffFile) == EOF) {
		perror("Error (Closing staff file) ");
		return 1;
	}
	return 0;
}


int staffPromptDetails(char* buf, enum StaffModifiableFields selection) {
	char* promptMessage;
	char format[] = "%    ";
	bool valid = false;

	switch(selection) {
		case ID:
			promptMessage = "ID (S000001)";
			strcpy(format+1, "7s");
			break;
		case NAME:
			promptMessage = "Name (John Smith)";
			strcpy(format+1, "127s");
			break;
		case POSITION:
			promptMessage = "Position (Administrator)";
			strcpy(format+1, "63s");
			break;
		case PERMISSION:
			// Special case.
			/*	NOTE:	Resulting permission binary number should be less than or equal to current logged in user.
						In other words, the current logged in user can't create a user that has permission they don't have.
						The current user may only turn off access to functions and not on.

						E.g. The logged in user with the following permission set.
							Hi        Lo
							0 0 0 0 1 1 = 3
							May only create user with only one of these following permissions.
							Hi        Lo		searchStaff()	addStaff()
							0 0 0 0 1 1 = 3			/				/
							0 0 0 0 1 0 = 2		   	/
							0 0 0 0 0 1 = 1							/
							0 0 0 0 0 0 = 0
			*/

			// XXX: An int array will be passed in so interpret it as int.
			int* oldPerm = (int*) buf;
			int* newPerm = (int*) (buf + sizeof(int));
			char tmpBuf[2];
			
			while(1) {
				printf("Access permission:\n");

				// Calculate how many functions are there.
				int numAccess = sizeof(ACCESS)/sizeof(ACCESS[0]);

				// Print each function with their current bit state.
				for(int i = 1; i <= numAccess; ++i) {
					printf("%d.\t%-63s\t%s\n", i, ACCESS[i-1], *newPerm & (1<<(i-1)) ? "ON" : "OFF");
				}
				printf("Type one of the numbers to toggle the staff's access to it.\n");
				printf("(Enter Q to quit)\n");

				// Don't reprint if it's just a small mistake.
				while(!valid) {
					printf("Selection (E.g. 2): ");

					if(scanf("%2s", tmpBuf) == EOF) {
						return EOF;
					}
					truncate();

					if(toupper(*tmpBuf) == 'Q') {
						// Exit char inputted.
						valid = true;
						break;
					}

					int permSelection = atoi(tmpBuf);

					if(permSelection == 0 && tmpBuf[0] != '0') {
						printf("Please enter a valid number!\n");
					} else if(permSelection-1 > numAccess) {
						printf("Plese enter a number smaller than or equal to %d!\n", numAccess+1);
					} else if(permSelection < 1) {
						printf("Please enter a number larger than or equal to 1!\n");
					} else if((*oldPerm & (1<<permSelection)) == 0) {
						printf("Insufficient permission!\n");
					} else {
						*newPerm ^= 1<<(permSelection-1);
						// Break out of the loop to print all of the codes again.
						break;
					}
				}
			}
			break;
		default:
			printf("An error occured! (Unimplemented)\n");
			return 1;
	};

	while(!valid) {
		printf("Enter staff's %s:", promptMessage);
		#undef PROMPT_MSG

		if(scanf(format, buf) == EOF) {
			return EOF;
		}
		truncate();

		// Validate or do nothing.
		valid = true;
		switch(selection) {
			case ID:
				if(buf[0] == 's') {
					// TODO: Should the system be case insensitive?
					valid = false;

					printf("Please ensure that the alphabet entered is in uppercase!\n");
					pause();
					break;
				} else if(buf[0] != 'S') {
					valid = false;
					printf("Invalid Staff ID format!\n");
					pause();
					break;
				}

				for(int i = 1; i <= 7; ++i) {
					if(buf[i] < '0' || buf[i] > '9') {
						printf("Invalid Staff ID format!\n");
						pause();
						valid = false;
						break;
					}
				}
				break;
			default:
				; // Do nothing.
		}
	}

	// No error.
	return 0;
}


int staffDisplaySelected(char** idList, int idListLen, enum StaffModifiableFields* displayList, int displayListLen) {
	FILE* staffFile = fopen("staff.bin", "r");

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		return 1;
	}

	// Allocate memory to hold a page of record.
	Staff* page = (Staff*) malloc(sizeof(Staff)*ENTRIES_PER_PAGE);
	if(page == NULL) {
		perror("Error (malloc)");
		// Designing a backup solution really will do me die ah.
		// TODO: Make a backup solution if malloc() fails.
		return 2;
	}

	int curPage = 1;

	while(1) {
		int read = 0;
		read = fread(page, sizeof(Staff), ENTRIES_PER_PAGE, staffFile);

		// ith index stores the width of dashes.
		// i+1th index stores the width of the column (including the dashes).
		short printWidth[STAFF_ENUM_LENGTH*2];
		int printWidthLen = 0;

		// Print headers.
		for(int i = 0; i < displayListLen; ++i) {
			switch(displayList[i]) {
				case SE_ID:
					printf("Staff ID  ");
					printWidth[i++] = 8;
					printWidth[i++] = 10;
					break;
				case SE_NAME:
					// Allocate a width of 52 cols for name.
					// 2 of the cols are for cols seperator and the rest are for the name.
					// If a name exceeds 47 characters (excluding null), ellipsis will be added.
					printf("Name                                                ");
					printWidth[i++] = 4;
					printWidth[i++] = 52;
					break;
				case SE_POSITION:
					printf("Position  ");
					printWidth[i++] = 8;
					printWidth[i++] = 30;
					break;
				case SE_PERMISSION:
					printf("Access  ");
					printWidthLen[i++] = 6;
					printWidthLen[i++] = 20;
					break;
				default:
					printf("?? ");
					printWidthLen[i++] = 2;
					printWidthLen[i++] = 2;
			}
		}
		putchar('\n');

		// Print header and content divider.
		for(int i = 0; i < printWidthLen; i += 2) {
			int ii = 0;
			for(; ii < printWidth[i]; ++ii) {
				putchar('-');
			}
			for(; ii < printWidth[i+1]; ++ii) {
				putchar(' ');
			}
		}
		putchar('\n');

		if(read == 0 && page == NULL) {
			printf("No more records!\n");
		else {
			int numPrinted = 0;
			int curPageCursor = 0;

			// Print staff details from $page.
			// $read will be 0 if $page is NULL.
			for(; page != NULL ? numPrinted < read : numPrinted < ENTRIES_PER_PAGE; ++numPrinted) {
				Staff rowToPrint;
				if(page != NULL) {
					rowToPrint = &page[curPageCursor];
				} else {
					int res = fread(&rowToPrint, sizeof(Staff), 1, staffFile);
					if(res != 1) {
						// EOF or error. Assume it's EOF.
						break;
					}
				}

				for(int ii = 0; ii < displayListLen; ++ii) {
					switch(displayList[ii]) {
						case SE_ID:
							printf("%s   ", rowToPrint.id);
							break;
						case SE_NAME:
							printf("%-47s", rowToPrint.name);
							if(strlen(rowToPrint.name) > 47) {
								printf("...");
							}
							printf("     ");
							break;
						case SE_POSITION:
							printf("%-27s", rowToPrint.position);
							if(strlen(rowToPrint.position) > 27) {
								printf("...");
							}
							printf("     ");
							break;
						case SE_PERMISSION:
							// TODO
							printf("Not implemented yet!  ");
						default:
							printf("??  ");
					}
				}
				putchar('\n');
			}
		}

		if(read <= 1) {
			printf("\nDisplaying %d entry", read);
		} else {
			printf("\nDisplaying %d entries", read);
		}


		if(len >= 0) {
			printf(" of %d entr%s.", len, len < 2 ? "y" : "ies");		
		}
		printf(" (Page %d)\n", curPage);

		printf("Enter N for next page, B to go back, Q to quit (default=N): ");

		char action;

		if(scanf("%1c", &action) == EOF) {
			return EOF;
		}
		truncate();

		// TODO: Don't allow user to forward or go backward if no more page.
		if(toupper(action) == 'Q') {
			break;
		} else if(toupper(action) == 'B') {
			// Rewind pointer to cur-1 page, current pointer is on cur+1 page.
			int seekBackRes = fseek(staffFile, -sizeof(Staff)*(ENTRIES_PER_PAGE+read), SEEK_CUR);

			if(seekBackRes != 0) {
				perror("Error (fseek back 2 page) ");
			} else {
				--curPage;
			}
		} else {
			// Do nothing since pointer is on the cur+1 page.
			++curPage;
		}
	}

	free(page);
	fclose(staffFile);

	if(idListLen < 0) {
	} else {

	}

	fclose(staffFile);
	return 0;
}


Staff staffLogin(void) {
	Staff s;
	char buf[MAX];
	FILE* staffFile = fopen("staff.bin", "rb");

	if(staffFile == NULL) {
		perror("Error (Opening staff file)");
		*((char*) &s) = -1; // Write to the first byte this way just in case first field of the struct is changed in the future.
		return s;
	}

	printf(
		"LOGIN\n"
		"=====\n"
	);

	while(1) {
		printf("Staff ID: ");

		// Any leftover characters after the 7th byte will be truncated.

		if(promptStaffDetails(buf, ID) == EOF) {
			*((char*) &s) = -1;
			return s;
		}
		truncate();

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
					*((char*) &s) = -1;
					return s;
				}
				truncate();

				if(computeHash(buf) == s.passHash) {
					// A valid password is entered.
					match = true;
					break;
				} else if(i > 0) {
					printf("Password incorrect! %d attempts left\n", i);
				}
			}

			if(match) {
				// Password matches, break out of login loop.
				break;
			} else {
				printf("Password and staff ID does not match! Please try again!\n\n");
			}
		} else {
			// Staff ID inputted not found in record, proceed to prompt again.
			printf("Staff ID not found!\n\n");
		}
		
	}

	fclose(staffFile);

	return s;
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

	#undef NN
	#undef BB

	return hash[0];
}


int main(void) {
	Staff a = {"S000000", {"ADMIN", "ADMIN"}, computeHash("ADMIN"), 0xFFFFFFFF};
	while(1) {
		char action;

		printf("actions = add, display, login: ");
		if(scanf("%1c", &action) == EOF) {
			break;
		}
		truncate();

		switch(toupper(action)) {
			case 'A':
				addStaff(&a);
				break;
			case 'D':
				displayStaff(&a);
				break;
			case 'L':
				login();
				break;
			default:
				// nothing
		}

	}
	// FILE* file = fopen("staff.bin", "wb");
	// fwrite(&a, sizeof(a), 1, file);
	// fclose(file);
	return 0;
}