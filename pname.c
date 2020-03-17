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

// #include "catalog/pg_collation.h"
// #include "utils/builtins.h"
// #include "utils/formatting.h"
#include "utils/hashutils.h"

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "c.h"  // include felxable memory size

PG_MODULE_MAGIC;

typedef struct PersonName
{
    int length;
	char *familyName;
	char *givenName;
} PersonName;


// define a function to check if name is valid
static bool 
checkName(char *name){
	int cflags = REG_EXTENDED;
	regex_t reg;
	bool ret = true;

	const char *pattern = "^[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*,[ ]?[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*$";
	regcomp(&reg, pattern, cflags);
	
	int status = regexec(&reg, name, 0, NULL, 0);
	
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
	char familyName[strlen(NameIn)], 
		 givenName[strlen(NameIn)],
		 punct[10];

	if (checkName(NameIn) == false){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type pname: \"%s\"", NameIn)));
	}

	// scan the input string and set familyName, givenName
	sscanf(NameIn, "%[A-Za-z' -]%[, ]%[A-Za-z' -]", familyName, punct, givenName);

	// set new pname object's size , +2 for 2"\0"
	int32 new_pname_size = strlen(familyName) + strlen(givenName) + 2;

	// assign memory to result
	PersonName *result = (PersonName *)palloc(VARHDRSZ + new_pname_size);

	// set VARSIZE
	SET_VARSIZE(result, VARHDRSZ + new_pname_size);

	// locate pointer
	result->familyName = result + VARHDRSZ；
	result->givenName = result->familyName + strlen(familyName) + 1;
	
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
 * Operator define
 *****************************************************************************/
static int 
compareNames(PersonName *a, PersonName *b){
	//waiting for Mark 
	int result = strcmp(a->familyName, b->familyName);

	if (result == 0){
		result = strcmp(a->givenName, b->givenName);
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
PG_FUNCTION_INFO_V1(pname_cmp)

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

PG_FUNCTION_INFO_V1(family);

Datum
family(PG_FUNCTION_ARGS) {
	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);

	char *family;
	family = psprintf("%s", fullname->familyName);
	PG_RETURN_CSTRING(family);
}

PG_FUNCTION_INFO_V1(given);

Datum
given(PG_FUNCTION_ARGS) {
	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);

	char *given;
	given = psprintf("%s", fullname->givenName);
	PG_RETURN_CSTRING(given);
}

PG_FUNCTION_INFO_V1(show);

Datum
show(PG_FUNCTION_ARGS) {
	PersonName *fullname = (PersonName *) PG_GETARG_POINTER(0);
	char *showName;
	char * firstGivenName;
	char *delim = " ";

	//get the first given name
	firstGivenName = strtok(fullname->givenName, delim); 
	showName = psprintf("%s %s", firstGivenName, fullname->familyName);

	PG_RETURN_CSTRING(showName);
}


/*****************************************************************************
 * Hash function define
 *****************************************************************************/
PG_FUNCTION_INFO_V1(pname_hash);

Datum
hash(PG_FUNCTION_ARGS)
{
	PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
	char       *str;
	Datum      result;

	str = psprintf("%s,%s", a->familyName, a->givenName);
	result = hash_any((unsigned char *) str, strlen(str));
	pfree(str);

	/* Avoid leaking memory for toasted inputs */
	PG_FREE_IF_COPY(a, 0);
	
	PG_RETURN_DATUM(result);
}