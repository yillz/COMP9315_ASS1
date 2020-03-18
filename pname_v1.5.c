/*
 * pname.c
 * author1: Yinghong Zhong(z5233608)
 * author2: Shaowei Ma(z5238010)
 * version:1.4
 * course: COMP9315
 * Item: Assignment1
*/
#include "postgres.h"

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>

#include "access/hash.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "c.h"  // include felxable memory size

PG_MODULE_MAGIC;

typedef struct PersonName
{
    int length;
	int fsize;
	int gsize;
	char pName[];
} PersonName;


// define a function to check if name is valid
static bool 
checkName(char *name){
	int cflags = REG_EXTENDED;
	regex_t reg;
	bool ret = true;

	const char *pattern = "^[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*,[ ]?[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*$";
	regcomp(&reg, pattern, cflags);
	
	int status;
	status = regexec(&reg, name, 0, NULL, 0);
	
	if (status != 0){
		ret = false;
	}
	regfree(&reg);
	return ret;
}


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

// Input function
PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
	char *NameIn = PG_GETARG_CSTRING(0);
	char *familyName, 
		 *givenName,
		 punct[10];

	if (checkName(NameIn) == false){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type pname: \"%s\"", NameIn)));
	}

	// palloc familuname and givenname
	familyName = palloc(strlen(NameIn)*sizeof(char));
	givenName = palloc(strlen(NameIn)*sizeof(char));
	

	// scan the input string and set familyName, givenName
	sscanf(NameIn, "%[A-Za-z' -]%[, ]%[A-Za-z' -]", familyName, punct, givenName);

	// set new pname object's size , +2 for 2"\0" 
	int32 new_pname_size = strlen(familyName) + strlen(givenName) + 2 + 2*sizeof(int);

	// assign memory to result
	PersonName *result = (PersonName *)palloc(VARHDRSZ + new_pname_size);
	result->fsize = strlen(familyName) + 1;
	result->gsize = strlen(givenName) + 1;

	// set VARSIZE
	SET_VARSIZE(result, VARHDRSZ + new_pname_size);
	
	// copy familyName and givenName to result
	memcpy(result->pName, familyName, result->fsize);
	memcpy(result->pName + result->fsize, givenName, result->gsize);
	
	pfree(familyName);
	pfree(givenName);

	PG_RETURN_POINTER(result);
}


// output function
PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);
	char  *result;

	result = psprintf("%s,%s", fullname->pName, fullname->pName + fullname->fsize);
	PG_RETURN_CSTRING(result);
}



/*****************************************************************************
 * Operator define
 *****************************************************************************/
static int 
compareNames(PersonName *a, PersonName *b){
	//waiting for Mark 
	int result = strcmp(a->pName, b->pName); 
	if (result == 0) {
		result = strcmp(a->pName + a->fsize, b->pName + b->fsize);
	}
	return result;
}

// 1. less function
PG_FUNCTION_INFO_V1(pname_less);

Datum
pname_less(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) < 0);
}

// 2. less equal function
PG_FUNCTION_INFO_V1(pname_less_equal);

Datum
pname_less_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) <= 0);
}

// 3. Equal function
PG_FUNCTION_INFO_V1(pname_equal);

Datum
pname_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) == 0);
}

// 4. not equal function
PG_FUNCTION_INFO_V1(pname_not_equal);

Datum
pname_not_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) != 0);
}

// 5. Greater function
PG_FUNCTION_INFO_V1(pname_greater);

Datum
pname_greater(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) > 0);
}

// 6. greater equal function
PG_FUNCTION_INFO_V1(pname_greater_equal);

Datum
pname_greater_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(compareNames(a, b) >= 0);
}

// 7.compare function
PG_FUNCTION_INFO_V1(pname_cmp);

Datum
pname_cmp(PG_FUNCTION_ARGS) 
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(compareNames(a, b));
}

/*****************************************************************************
 * Functions
 *****************************************************************************/

// PG_FUNCTION_INFO_V1(family);

// Datum
// family(PG_FUNCTION_ARGS) {
// 	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);

// 	char *family;
// 	family = psprintf("%s", fullname->pName);
// 	PG_RETURN_CSTRING(family);
// }

// PG_FUNCTION_INFO_V1(given);

// Datum
// given(PG_FUNCTION_ARGS) {
// 	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);

// 	char *given;
// 	given = psprintf("%s", fullname->pName + strlen(fullname->pName) + 1);
// 	PG_RETURN_CSTRING(given);
// }

// PG_FUNCTION_INFO_V1(show);

// Datum
// show(PG_FUNCTION_ARGS) {
// 	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);
// 	char *showName;
// 	char *fullcopy;
// 	char *firstGivenName;
// 	char *delim = " ";

// 	strcpy(fullcopy, fullname->pName);

// 	//get the first given name
// 	firstGivenName = strtok(fullcopy + strlen(fullcopy) + 1, delim); 

// 	showName = psprintf("%s %s", firstGivenName, fullname->pName);

// 	PG_RETURN_CSTRING(showName);
// }


// /*****************************************************************************
//  * Hash function define
//  *****************************************************************************/
// PG_FUNCTION_INFO_V1(pname_hash);

// Datum
// pname_hash(PG_FUNCTION_ARGS)
// {
// 	PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
// 	char       *str;
// 	int32      result;

// 	char *fname_pointer = a->pName;
// 	char *gname_pointer = a->pName + strlen(a->pName) + 1;

// 	str = psprintf("%s,%s", fname_pointer, gname_pointer);
// 	result = DatumGetUInt32(hash_any((unsigned char *) str, strlen(str)));
// 	pfree(str);

// 	/* Avoid leaking memory for toasted inputs */
// 	PG_FREE_IF_COPY(a, 0);
	
// 	PG_RETURN_INT32(result);
// }