/* test_mbsrtowcs.c
** Test multi-byte character to wide character conversion routines (reentrant version)
** cc -o test_mbsrtowcs test_mbsrtowcs.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	mbstate_t mbs_state;
	int i,retval;

	if(argc < 2)
	{
		fprintf(stderr,"test_mbsrtowcs : Test multi-byte character to wide character conversion routines.\n");
		fprintf(stderr,"test_mbsrtowcs <string> [<string>]\n");
		return 1;
	}
	for(i = 1; i < argc; i++)
	{
		wprintf(L"Converting parameter %d '%s'\n",i,argv[i]);
		memset(&mbs_state,0, sizeof(mbstate_t));
		retval = mbsrtowcs(wide_char_string,(const char **)&(argv[i]),MAX_STRING_LENGTH,&mbs_state);
		wprintf(L"Converted parameter %d from '%s' to '%ls' with retval %d.\n",i,argv[i],
			wide_char_string,retval);
		wprintf(L"Converted parameter %d and returned %d.\n",i,retval);
	}
	return 0;
}
