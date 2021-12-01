/* ccd_buffer.c
** Moptop PCO CCD library
*/
/**
 *  Routines to control the output buffers.
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_command.h"
#include "ccd_setup.h"
#include "ccd_buffer.h"

/* hash defines */

/* data types */
/**
 * Data type holding local data to ccd_buffer. This consists of the following:
 * <dl>
 * <dt>Image_Size_Bytes</dt> <dd>The image size in bytes, used to allocate image buffers in Image_Buffer_List.
 *                           Note extra space is allocated for the meta-data.</dd>
 * <dt>Image_Buffer</dt> <dd>An allocated image buffer.</dd>
 * </dl>
 * @see #BUFFER_COUNT
 */
struct Buffer_Struct
{
	int Image_Size_Bytes;
	int *Image_Buffer;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Buffer_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Image_Size_Bytes</dt> <dd>0</dd>
 * <dt>Image_Buffer</dt> <dd>NULL</dd>
 * </dl>
 */
static struct Buffer_Struct Buffer_Data = 
{
	0,NULL
};

/**
 * Variable holding error code of last operation performed.
 */
static int Buffer_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see ccd_general.html#CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Buffer_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Create the image buffer used to store read out data from the CCD.
 * <ul>
 * <li>We set the image buffer Buffer_Data.Image_Buffer to NULL.
 * <li>We call CCD_Setup_Dimensions with binning 1 to configure the PCO library. This also sets up the setup
 *     module to return the image size in bytes for binning 1.
 * <li>We call CCD_Setup_Get_Image_Size_Bytes to get the image size in bytes for binning 1 from the setup module.
 *     We store this in Buffer_Data.
 * <li>We allocate Buffer_Data.Image_Size_Bytes memory for the image buffer, 
 *     and store the allocated pointer in Buffer_Data.Image_Buffer. 
 * </ul>
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #BUFFER_COUNT
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see #Buffer_Data
 * @see ccd_general.html#CCD_General_Log_Format
 * @see ccd_setup.html#CCD_Setup_Dimensions
 * @see ccd_setup.html#CCD_Setup_Get_Image_Size_Bytes
 */
int CCD_Buffer_Initialise(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Buffer_Initialise: Started.");
#endif /* LOGGING */
	/* initialise the image buffer pointer to NULL */
	Buffer_Data.Image_Buffer = NULL;
	/* set the binning to 1. This also updates setup's image size in bytes. */
	if(!CCD_Setup_Dimensions(1))
	{
		Buffer_Error_Number = 1;
		sprintf(Buffer_Error_String,"CCD_Buffer_Initialise: CCD_Setup_Dimensions failed.");
		return FALSE;
	}
	/* save the pco library computed image size in bytes. This takes account of the binning (set
	** to 1 above) */
	Buffer_Data.Image_Size_Bytes = CCD_Setup_Get_Image_Size_Bytes();
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Buffer_Initialise: Binned 1 image size %d bytes.",
			       Buffer_Data.Image_Size_Bytes);
#endif /* LOGGING */
	/* allocate buffers */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Buffer_Initialise: Allocating image buffer of %d bytes size.",
			       Buffer_Data.Image_Size_Bytes);
#endif /* LOGGING */
	Buffer_Data.Image_Buffer = (int *)malloc(Buffer_Data.Image_Size_Bytes* sizeof(int));
	if(Buffer_Data.Image_Buffer == NULL)
	{
		Buffer_Error_Number = 2;
		sprintf(Buffer_Error_String,"CCD_Buffer_Initialise: Allocating %d bytes failed.",
			Buffer_Data.Image_Size_Bytes);
		return FALSE;
	}
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"CCD_Buffer_Initialise: Buffer = %p.",
				       Buffer_Data.Image_Buffer);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Buffer_Initialise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Free previously allocated buffers.
 * <ul>
 * <li>We free the  allocated data pointed to by Buffer_Data.Image_Buffer and set the pointer to NULL.
 * </ul>
 * @see #Buffer_Data
 */
int CCD_Buffer_Free(void)
{
	if(Buffer_Data.Image_Buffer != NULL)
		free(Buffer_Data.Image_Buffer);
	Buffer_Data.Image_Buffer = NULL;
	return TRUE;
}

/**
 * Return the pointer used to store read out CCD images.
 * @return A pointer to the allocated memory, stored in Buffer_Data.Image_Buffer.
 * @see #CCD_Buffer_Get_Image_Buffer
 */
void *CCD_Buffer_Get_Image_Buffer(void)
{
	return Buffer_Data.Image_Buffer;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Buffer_Error_Number
 */
int CCD_Buffer_Get_Error_Number(void)
{
	return Buffer_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Buffer_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Buffer_Error_Number == 0)
		sprintf(Buffer_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Buffer:Error(%d) : %s\n",time_string,
		Buffer_Error_Number,Buffer_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Buffer_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Buffer_Error_Number == 0)
		sprintf(Buffer_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Exposure:Error(%d) : %s\n",time_string,
		Buffer_Error_Number,Buffer_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
