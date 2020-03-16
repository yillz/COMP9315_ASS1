/*
 * pname.c
 * author1: Yinghong Zhong(z5233608)
 * author2: Shaowei Ma(z5238010)
 * version:1.3
 * course: COMP9315
 * Item: Assignment1
*/

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "c.h"  // include felxable memory size

PG_MODULE_MAGIC;

typedef struct PersonName
{
    int length;
	// char familyName[FLEXIBLE_ARRAY_MEMBER];
	// char givenName[FLEXIBLE_ARRAY_MEMBER];
	char *familyName;
	char *givenName;
} PersonName;


// define a function to check if name is valid
static bool checkName(char *name){
	int cflags = REG_EXTENDED;
	regex_t reg;
	bool ret = true;

	const char *pattern = "^[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*,[ ]?[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*$";
	regcomp(&reg, pattern, cflags);
	
	int status = regexec(&reg, name, 0, NULL, 0);
	
	if (status != 0){
		ret = false;
	}
	regfree(&regex);
	return ret;
}

// remove the first space of a name if its first char is a space
/*static char *removeFirstSpace(char *name){
	char *new_name = name;
	if (*name == ' '){
		new_name++;
	}
	return new_name;
}*/

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

// Input function
PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
	char *NameIn = PG_GETARG_CSTRING(0);
	char familyName[100], 
		 givenName[100],
		 punct[10];

	if (checkName(NameIn) == false){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type pname: \"%s\"", NameIn)));
	}

	// scan the input string and set familyName, givenName
	sscanf(NameIn, "%[A-Za-z' -]%[, ]%[A-Za-z' -]", familyName, punct, givenName);

	// check if the given name start with a space
	//givenName = removeFirstSpace(givenName);

	// set new pname object's size , +2 for 2"\0"
	int32 new_pname_size = strlen(familyName) + strlen(givenName) + 2;

	// assign memory to result
	PersonName *result = (PersonName *)palloc(VARHDRSZ + new_pname_size);

	// set VARSIZE
	SET_VARSIZE(result, VARHDRSZ + new_pname_size);

	// memcopy
	// memcpy(VARDATA(result), VARDATA_ANY(familyName), (strlen(familyName) + 1));
	// memcpy(VARDATA(result) + strlen(familyName) + 1, VARDATA_ANY(givenName), (strlen(givenName) + 1));

	// locate pointer
	result->familyName = result + VARHDRSZ；
	result->givenName = result->familyName + strlen(familyName) + 1；
	
	// copy familyName and givenName to result
	strcpy(result->familyName, familyName);
	strcpy(result->givenName, givenName);
	PG_RETURN_POINTER(result);
}


// output function
PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);
	char  *result;

	result = psprintf("%s,%s", fullname->familyName, fullname->givenName);
	PG_RETURN_CSTRING(result);
}



/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

// function to compare strings
static int compareNames(PersonName *a, PersonName *b){
	//waiting for Mark 
	int result = strcmp(a->familyName, b->familyName);

	if (result == 0){
		result = strcmp(a->givenName, b->givenName);
	}

	return result;
}

// 1. Equal function
PG_FUNCTION_INFO_V1(pname_equal);

Datum
pname_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) == 0);
}

// 2. Greater function
PG_FUNCTION_INFO_V1(pname_greater);

Datum
pname_greater(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) > 0);
}

// 3. not equal function
PG_FUNCTION_INFO_V1(pname_not_equal);

Datum
pname_not_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) != 0);
}

// 4. greater equal function
PG_FUNCTION_INFO_V1(pname_greater_equal);

Datum
pname_greater_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) >= 0);
}

// 5. less function
PG_FUNCTION_INFO_V1(pname_less);

Datum
pname_less(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) < 0);
}

// 56. less equal function
PG_FUNCTION_INFO_V1(pname_less_equal);

Datum
pname_less_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) <= 0);
}
