/* test_mbstowcs.c
** Test multi-byte character to wide character conversion routines
** cc -o test_mbstowcs test_mbstowcs.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

/**
 * Maximum length of string (1 parameter) to convert.
 */
#define MAX_STRING_LENGTH   (256)

/**
 * Main program.
 * @param argc The number of arguments.
 * @param argv An array of argument strings.
 * @return Main returns 0 on success and a postive integer on failure.
 */
int main(int argc, char *argv[])
{
	wchar_t wide_char_string[MAX_STRING_LENGTH];
	int i,retval;

	if(argc < 2)
	{
		fprintf(stderr,"test_mbstowcs : Test multi-byte character to wide character conversion routines.\n");
		fprintf(stderr,"test_mbstowcs <string> [<string>]\n");
		return 1;
	}
	for(i = 1; i < argc; i++)
	{
		wprintf(L"Converting parameter %d '%s'\n",i,argv[i]);
		retval = mbstowcs(wide_char_string,argv[i],MAX_STRING_LENGTH);
		wprintf(L"Converted parameter %d to '%ls' with retval %d.\n",i,wide_char_string,retval);
		wprintf(L"Converted parameter %d and returned %d.\n",i,retval);
	}
	return 0;
}
