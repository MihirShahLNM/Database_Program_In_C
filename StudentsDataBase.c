#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>

// StudentsDataBase V5 by Mihir dated 06-May-2020

typedef struct struct_student {
	int roll;
	char name[20];
	float marks;
	int isDeleted;
} ent_student;

typedef struct struct_index {
	int roll;
	int EntryNo;
	int isDeleted;
} idx_student;

#define entFile "entry.db"
#define idxFile "index.idx"
#define tmpFile "temp.db"
#define backFile "entry.bak"

static int displayDeleted = 0;

int copyFile(char sourcePath[], char destPath[])
{
	FILE *sourceFile, *destFile;
    char ch;

    /* 
     * Open source file in 'r' and 
     * destination file in 'w' mode 
     */
    sourceFile  = fopen(sourcePath, "r");
    destFile    = fopen(destPath,   "w");

    /* fopen() return NULL if unable to open file in given mode. */
    if (sourceFile == NULL || destFile == NULL)
    {
        /* Unable to open file hence exit */
        printf("\nUnable to open file.\n");
        printf("Please check if file exists and you have read/write privilege.\n");

		return 1;
    }


    /*
     * Copy file contents character by character.
     */
    ch = fgetc(sourceFile);
    while (ch != EOF)
    {
        /* Write to destination file */
        fputc(ch, destFile);

        /* Read next character from source file */
        ch = fgetc(sourceFile);
    }

    /* Finally close files to release resources */
    fclose(sourceFile);
    fclose(destFile);

    return 0;
}

int sort()
{
	// called by add(), re-index(), purge().
	
	int i = 0, corrected = 1, TotalRecords = 0;
	FILE *fp_idx; 
	idx_student idx1, idx2;
	
	fp_idx = fopen(idxFile, "r+b");

	printf(" Sorting");
	
	///////////////////
	while (corrected == 1)	/// Re-run if record corrected
	{
		corrected = 0;
		rewind(fp_idx);		/// << restart from the bigining
		i++;
		printf(" Loop# %d, ", i);
		
		// printf("Step 0: %d",ftell(fp_idx));
		/// Read the first record
		while (fread(&idx1, sizeof(idx1), 1, fp_idx))
		{
			// printf("Step 1: %d ",ftell(fp_idx));
			/// Read the next record
			if (fread(&idx2, sizeof(idx2), 1, fp_idx))
			{
				// printf("Step 2: %d ",ftell(fp_idx));
				if (idx1.roll > idx2.roll)
				{
					// printf("\nCorrected 1:Roll=%d,  2:Roll=%d",idx1.roll ,idx2.roll);
					corrected = 1;
					fseek(fp_idx, sizeof(idx2) * (-2), SEEK_CUR); 	/// Roll back 2 records
					if (!(fwrite(&idx2, sizeof(idx2), 1, fp_idx)))
						printf("\n\nIndex mismatch !!! Reindex needed.");
					if (!(fwrite(&idx1, sizeof(idx1), 1, fp_idx)))
						printf("\n\nIndex mismatch !!! Reindex needed.");
				} else 
				{
					/// No correction needed
					// printf("\n          1:Roll=%d,  2:Roll=%d",idx1.roll ,idx2.roll);
				}
				fseek(fp_idx, sizeof(idx1) * (-1), SEEK_CUR); 	/// Roll back 1 record
				// printf("Step 3: %d ",ftell(fp_idx));
			} else	/// there is no next record
			{
				// printf(">> Last Record.\n");
				
			}
		}
		// printf ("Loop over with corrected %d\n\n", corrected);
		// getch();
	}
	fclose(fp_idx);
	
	printf(" Done.\n\n");
	return 0;
}

int reIndex()
{
	int entryNo = 0;
	ent_student ent1;
	idx_student idx1;
	FILE *fp_ent, *fp_idx;
	
	printf("\nPlease wait while re-indexing ...");
	unlink(idxFile);
	fp_ent = fopen(entFile, "rb");
	fp_idx = fopen(idxFile, "a+b");
	
	while (fread(&ent1, sizeof(ent1), 1, fp_ent))
	{	
		if ((ent1.isDeleted) != 1) entryNo++;
		// printf("\nNew Record: %d", entryNo);
		// printf("\nent1.roll: %d, ent1.Entry:%d", ent1.roll, entryNo);
			
		idx1.roll = ent1.roll;
		idx1.EntryNo = entryNo;
		idx1.isDeleted = ent1.isDeleted;
		
		//fseek(fp_idx, sizeof(idx1) * (entryNo - 1)), SEEK_SET);
		fwrite(&idx1, sizeof(idx1), 1, fp_idx);
		// printf(" <==>  idx1.roll: %d, idx1.Entry:%d", idx1.roll, idx1.EntryNo);
	}
	fclose(fp_ent);
	fclose(fp_idx);
	
	// printf("\n\nNow Sorting");
	sort();
	// printf("\n\n Done");
	
	return 0;
}

int search(int r_n)
{
	int flag = 0, TotalRecords = 0, SeekEntryNo = 0;
	FILE *fp_ent, *fp_idx; 
	ent_student ent1;
	idx_student idx1;
	
	fp_ent = fopen(entFile, "r+b");
	fp_idx = fopen(idxFile, "r+b");
	
	/// Build the logic to search faster
	/// Count no of records in Index
	fseek(fp_idx, 0, SEEK_END);
	TotalRecords = (ftell(fp_idx) / sizeof(idx1));
	printf(" (%d / %d = #%d) ", ftell(fp_idx), sizeof(idx1), TotalRecords);
	rewind(fp_idx);		/// << restart from the bigining
	if (TotalRecords > 50)
	{
		SeekEntryNo = TotalRecords / 2;				  /// Seek middle of the database
		printf(" Seek %d ", SeekEntryNo);
		fseek(fp_idx, sizeof(idx1) * SeekEntryNo, SEEK_SET);
		if (fread(&idx1, sizeof(idx1), 1, fp_idx))
		{
			if (idx1.roll < r_n)
			{
				/// Go ahead to while loop to search the 2d half
			} else
			{
				/// rewind & go to while loop to search the 1st half
				rewind(fp_idx);		/// << restart from the bigining
			}
		}
	} 
	
	/// While loop searches sequencially from current pointer to find the record.
	while (fread(&idx1, sizeof(idx1), 1, fp_idx))
	{
		/// printf("\n\nSeeking idx1.roll:%d, idx1.EntryNo:%d", idx1.roll, idx1.EntryNo);
		if ((idx1.roll == r_n) && (idx1.isDeleted != 1))		/// Search Roll to Get EntryNo from index 
		{
			flag = 1;
			break;
		}
	}
		
	/// Print record if found 
	if (flag == 1)
	{
		// printf("\n\nFound idx1.roll:%d, idx1.EntryNo:%d", idx1.roll, idx1.EntryNo);
		/// Get EntryNo from index & then seek same EntryNo in main database.
		fseek(fp_ent, sizeof(ent1) * (idx1.EntryNo - 1), SEEK_SET);
		fread(&ent1, sizeof(ent1), 1, fp_ent);
		printf("\n\nRoll :%d", ent1.roll);
		printf("\n\nName :%s", ent1.name);
		printf("\n\nMarks :%f", ent1.marks);
	}
	printf("\n\n");
	
	fclose(fp_ent);
	fclose(fp_idx);
	
	return flag;
}

int search_slow(int r_n)
{
	int flag = 0;
	FILE *fp_ent, *fp_idx; 
	ent_student ent1;
	idx_student idx1;
	
	fp_ent = fopen(entFile, "r+b");
	fp_idx = fopen(idxFile, "r+b");
	
	// Build the logic to search faster
	
	
	while (fread(&idx1, sizeof(idx1), 1, fp_idx))
	{
		/// printf("\n\nSeeking idx1.roll:%d, idx1.EntryNo:%d", idx1.roll, idx1.EntryNo);
		if ((idx1.roll == r_n) && (idx1.isDeleted != 1))		/// Search Roll to Get EntryNo from index 
		{
			flag = 1;
			break;
		}
	}
	if (flag == 1)
	{
		// printf("\n\nFound idx1.roll:%d, idx1.EntryNo:%d", idx1.roll, idx1.EntryNo);
		/// Get EntryNo from index & then seek same EntryNo in main database.
		fseek(fp_ent, sizeof(ent1) * (idx1.EntryNo - 1), SEEK_SET);
		fread(&ent1, sizeof(ent1), 1, fp_ent);
		printf("\n\nRoll :%d", ent1.roll);
		printf("\n\nName :%s", ent1.name);
		printf("\n\nMarks :%f", ent1.marks);
	}
	printf("\n\n");
	
	fclose(fp_ent);
	fclose(fp_idx);
	
	return flag;
}

int add()
{
	int n = 0, LastEntryNo = 0;
	float temp;
	FILE *fp_ent, *fp_idx; 
	ent_student ent1;
	idx_student idx1;
	
	printf("\n\nEnter roll no: ");
	scanf("%d", &ent1.roll);
	printf("\nEnter Name: ");
	scanf("%s", ent1.name);
	printf("\nEnter marks: ");
	scanf("%f", &temp);
	ent1.marks = temp;
	ent1.isDeleted = 0;
	
	/// Check if Roll No already exists !!!
	/// Use Search()
	if (search(ent1.roll) == 1)
	{
		return 1;
	}
	
	fp_ent = fopen(entFile, "r+b");
	fp_idx = fopen(idxFile, "r+b");
	
	/// Count no of records in Index
	// while ((fread(&idx1, sizeof(idx1), 1, fp_idx)))
	 	// LastEntryNo++;
	fseek(fp_ent, 0, SEEK_END);
	LastEntryNo = (ftell(fp_ent) / sizeof(ent1));
	printf("\nNew Entry#%d\n", LastEntryNo+1);
	
	if (fwrite(&ent1, sizeof(ent1), 1, fp_ent))
	{
		LastEntryNo++;
		fseek(fp_idx, 0, SEEK_END);
		idx1.roll = ent1.roll;
		idx1.EntryNo = LastEntryNo;
		idx1.isDeleted = 0;
		if (!(fwrite(&idx1, sizeof(idx1), 1, fp_idx)))
			printf("Index mismatch. Reindex needed.\n");
	}
	else
	{
		printf(" Could NOT add !!!\n");
	}

	fclose(fp_idx);
	fclose(fp_ent);

	//// Sort the Index
	sort();
	return 0;
}

int edit(int r_n)
{
	int flag = 0;
	float temp;
	FILE *fp_ent, *fp_idx; 
	ent_student ent1;
	idx_student idx1;
	
	/// Check if Roll No exists !!!
	/// Use Search()
	if (search(r_n) != 1)
	{
		flag = 1;
		return flag;
	}
	
	fp_ent = fopen(entFile, "r+b");
	fp_idx = fopen(idxFile, "r+b");

	while (fread(&idx1, sizeof(idx1), 1, fp_idx))
	{
		if ((idx1.roll == r_n) && (idx1.isDeleted != 1))
		{
			flag = 1;
			break;
		}
	}
	if (!flag)
	{
		printf("\n\nRecord NOT found !!!");
	}
	else
	{
		/// Get EntryNo from index & then seek same EntryNo in main database.
		fseek(fp_ent, sizeof(ent1) * (idx1.EntryNo - 1), SEEK_SET);
		fread(&ent1, sizeof(ent1), 1, fp_ent);
		printf("\nRoll :%d", ent1.roll);
		printf("\nName :%s", ent1.name);
		printf("\nMarks :%f", ent1.marks);

		printf("\n\n\nEnter New Name : ");
		scanf("%s", &ent1.name);
		printf("\nEnter New marks : ");
		scanf("%f", &temp);
		ent1.marks = temp;
		fseek(fp_ent, sizeof(ent1) * (-1), SEEK_CUR); 	/// Roll back 1 record
		if((fwrite(&ent1, sizeof(ent1), 1, fp_ent)) != 1)
			printf("\n\nCould NOT be modified !!!");
	}
	printf("\n\n");
	
	fclose(fp_ent);
	fclose(fp_idx);

	return flag;
}

int display_index()
{
	int flag = 1;
	idx_student idx1;
	FILE *fp_idx;
	
	fp_idx = fopen(idxFile, "rb");
	
	while (fread(&idx1, sizeof(idx1), 1, fp_idx))
	{
		if ((idx1.roll) != 0)
		{
			// printf("\n 			Para:%d", displayDeleted);
			// printf("	 Idx:%d", idx1.isDeleted);
			if ((idx1.isDeleted != 1) | (displayDeleted == 1))
			{
				if (flag == 1)
					printf("\n\nRoll_No EntryNo Del");	/// Print heading only once
				flag = 0;
				printf("\n\n%d", idx1.roll);
				printf(" #%d", idx1.EntryNo);
				printf(" %s", (idx1.isDeleted == 1) ? "*" : " " );
			}
		}
	}
	
	if (flag == 1)
		printf("\n\nNo record found !!!");
	printf("\n\n");
	
	fclose(fp_idx);
	return flag;
}

int display()
{
	int flag = 1, record = 0;
	ent_student ent1;
	FILE *fp_ent;
	
	fp_ent = fopen(entFile, "rb");
	
	while (fread(&ent1, sizeof(ent1), 1, fp_ent))
	{
		record++;
		if ((ent1.roll) != 0)
		{
			// printf("\n 			Para:%d", displayDeleted);
			// printf("	 Data:%d", ent1.isDeleted);
			if ((ent1.isDeleted != 1) | (displayDeleted == 1))

			{
				if (flag == 1)
					printf("\n\n# Roll Name  Marks  Del");	/// Print heading only once
				flag = 0;
				printf("\n\n#%d", record);
				printf(" %d", ent1.roll);
				printf(" %s", ent1.name);
				printf(" %f", ent1.marks);
				printf(" %s", (ent1.isDeleted == 1)? "*" : " " );
			}
		}
	}
	
	if (flag == 1)
		printf("\n\nNo record found !!!");
	printf("\n\n");
	
	fclose(fp_ent);
	return flag;
}

int purge()
{
	ent_student ent1;
	idx_student idx1;
	FILE *fp_ent, *fp_temp, *fp_idx;
	int entryNo = 0;
	
	printf("\n\nPlease wait while Purging ... \n");

	unlink(idxFile);
	unlink(tmpFile);
	fp_ent = fopen(entFile, "rb");
	fp_temp = fopen(tmpFile, "a+b");
	fp_idx = fopen(idxFile, "a+b");
	
	
	while (fread(&ent1, sizeof(ent1), 1, fp_ent))
	{
		printf("Adding Rec#%d, ", entryNo + 1);
		if ((ent1.isDeleted) != 1)
		{
			entryNo++;
			idx1.roll = ent1.roll;
			idx1.EntryNo = entryNo;
			idx1.isDeleted = ent1.isDeleted;
			
			fseek(fp_temp, 0, SEEK_END);
			/// fseek(fp_temp, sizeof(ent1) * ((ent1.roll) - 1), SEEK_SET);
			// fseek(fp_temp, sizeof(ent1) * (entryNo - 1), SEEK_SET);
			if (!(fwrite(&ent1, sizeof(ent1), 1, fp_temp)))
				printf("\n\nData mismatch !!! Restore needed.");
			fseek(fp_idx, 0, SEEK_END);
			// fseek(fp_idx, sizeof(idx1) * (entryNo - 1), SEEK_SET);
			if (!(fwrite(&idx1, sizeof(idx1), 1, fp_idx)))
				printf("\n\nIndex mismatch !!! Reindex needed.");
		} else
		{
			printf(" Deleted");
		}
	}
	fclose(fp_ent);
	fclose(fp_idx);
	fclose(fp_temp);
	unlink(backFile);
	rename(entFile, backFile);
	rename(tmpFile, entFile);
	
	printf("Done.");
	printf("\n");
	sort();
	
	return 0;
}

int delete_rec(int r_n)
{
	int flag = 0;
	float temp;
	FILE *fp_ent, *fp_idx; 
	ent_student ent1;
	idx_student idx1;
	
	fp_ent = fopen(entFile, "r+b");
	fp_idx = fopen(idxFile, "r+b");

	/// Check if Roll No already exists !!!
	/// Use Search()

	while (fread(&idx1, sizeof(idx1), 1, fp_idx))
	{
		if (idx1.roll == r_n)
		{
			flag = 1;
			break;
		}
	}
	if (!flag)
	{
		printf("\n\nRecord NOT found !!!");
	}
	else
	{
		/// Get EntryNo from index & then seek same EntryNo in main database.
		fseek(fp_ent, sizeof(ent1) * (idx1.EntryNo - 1), SEEK_SET);
		fread(&ent1, sizeof(ent1), 1, fp_ent);
		printf("\n\nRoll :%d", ent1.roll);
		printf("\nName :%s", ent1.name);
		printf("\nMarks :%f", ent1.marks);
		ent1.isDeleted = 1;
		idx1.isDeleted = 1;
		printf("\n		deleted");

		fseek(fp_ent, sizeof(ent1) * (-1), SEEK_CUR); 	/// Roll back 1 record
		if((fwrite(&ent1, sizeof(ent1), 1, fp_ent)) != 1)
			printf("\n\n	Could NOT be deleted !!!");
		fseek(fp_idx, sizeof(idx1) * (-1), SEEK_CUR); 	/// Roll back 1 record
		if((fwrite(&idx1, sizeof(idx1), 1, fp_idx)) != 1)
			printf("\n\n	Index not updated !!! Please re-index.");
	}
	printf("\n\n");
	
	fclose(fp_ent);
	fclose(fp_idx);

	return flag;
}

int main()
{
	int choice = 0, r_n = 0;
	FILE *fp;
	
	// clrscr();
	fp = fopen(entFile , "a+b");
	fclose(fp);
	fp = fopen(idxFile, "a+b");
	fclose(fp);
	// fp = fopen(tmpFile, "a+b");
	// fclose(fp);
	
	/// Check no of records in Data-Entry file & no of records in Index.
	/// If any mis-match, insist to run re-index.
	
	while (1)
	{
		printf("\n\n1:Add 2:Display 3:Display_Index 4:Reindex 5:Search 6:Edit 7:Delete 8:Purge 9:DisplayDeleted 0:Exit \n\n");
		scanf("%d", &choice);
		
		// if (isdigit(choice)) 
		// {
		switch (choice)
		{
			case 1:
				if (add() == 1)
					printf("Roll No already exists. Aborting ADD.");
				break;
			case 2:
				display();
				break;
			case 3:
				display_index();
				break;
			case 4:
				reIndex();
				break;
			case 5:
				printf("\n\nEnter Roll No To Search Record : ");
				scanf("%d", &r_n);
				if (search(r_n) == 0)
					printf("\n\nRecord NOT found !!!");
				break;
			case 6:
				printf("\n\nEnter Roll No To Edit Record : ");
				scanf("%d", &r_n);
				edit(r_n);
				break;
			case 7:
				printf("\n\nEnter Roll No To Delete Record : ");
				scanf("%d", &r_n);
				delete_rec(r_n);
				break;
			case 8:
				purge();
				break;
			case 9:
				switch (displayDeleted)
				{
					case 0:
						displayDeleted  = 1;
						printf("\n\nDeleted message will be displayed.");
						break;
					case 1:
						displayDeleted  = 0;
						printf("\n\n Deleted message will NOT be displayed.");
						break;
					case 2:
						displayDeleted  = 0;
						printf("\n\nDeleted message will NOT be displayed.");
						break;
				}
				break;
			case 0:
				return 0;	// exit(1);
				break;
			default:
				printf("\n\nInvalid choice !!!");
				break;
		}
		// } else
		// {
		//	 printf("Invalid input.");
		// }
	}
	// getch();
	return 0;
}

