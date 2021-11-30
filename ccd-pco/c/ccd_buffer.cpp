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
/**
 * Maximum number of PI rotator revolutions.
 */
#define BUFFER_MAX_ROTATOR_REVOLUTION_COUNT   (100)
/**
 * Maximum number of images per revolution.
 */
#define BUFFER_MAX_IMAGES_PER_REVOLUTION      (16)
/**
 * Number of image buffers in the ring buffer.
 * @see #BUFFER_MAX_IMAGES_PER_REVOLUTION
 */
#define BUFFER_COUNT                          (BUFFER_MAX_IMAGES_PER_REVOLUTION)

/* data types */
/**
 * Data type holding local data to ccd_buffer. This consists of the following:
 * <dl>
 * <dt>Image_Size_Bytes</dt> <dd>The image size in bytes, used to allocate image buffers in Image_Buffer_List.
 *                           Note extra space is allocated for the meta-data.</dd>
 * <dt>Image_Buffer_List</dt> <dd>A list of pointers to allocated image buffers. The list has BUFFER_COUNT entries.</dd>
 * </dl>
 * @see #BUFFER_COUNT
 */
struct Buffer_Struct
{
	int Image_Size_Bytes;
	int *Image_Buffer_List[BUFFER_COUNT]; /* diddly TODO */
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
 * <dt>Image_Buffer_List</dt> <dd>{NULL}</dd>
 * </dl>
 */
static struct Buffer_Struct Buffer_Data = 
{
	0,
	{NULL}
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
 * Create a sequence of buffers used to store read out data from the CCD.
 * <ul>
 * <li>We set all (BUFFER_COUNT) the image buffers in Buffer_Data.Image_Buffer_List to NULL.
 * <li>We call CCD_Setup_Dimensions with binning 1 to configure the PCO library. This also sets up the setup
 *     module to return the image size in bytes for binning 1.
 * <li>We call CCD_Setup_Get_Image_Size_Bytes to get the image size in bytes for binning 1 from the setup module.
 *     We store this in Buffer_Data.
 * <li>We call CCD_Setup_Get_Sensor_Width and CCD_Setup_Get_Sensor_Height to get the sensor size, and print
 *     out some information on image sizes.
 * <li>We loop over all (BUFFER_COUNT) the image buffers and allocate Buffer_Data.Image_Size_Bytes memory for each
 *     buffer, and stored in Buffer_Data.Image_Buffer_List. We use aligned_alloc to ensure the buffer is aligned
 *     to an 8 byte boundary (as required by Andor).
 * </ul>
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #BUFFER_COUNT
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see #Buffer_Data
 * @see ccd_general.html#CCD_General_Log_Format
 * @see ccd_setup.html#CCD_Setup_Dimensions
 * @see ccd_setup.html#CCD_Setup_Get_Image_Size_Bytes
 * @see ccd_setup.html#CCD_Setup_Get_Sensor_Width
 * @see ccd_setup.html#CCD_Setup_Get_Sensor_Height
 */
int CCD_Buffer_Initialise(void)
{
	int i,sensor_width,sensor_height;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Buffer_Initialise: Started.");
#endif /* LOGGING */
	/* initialise all image buffer pointers to NULL */
	for(i=0; i < BUFFER_COUNT; i++)
	{
		Buffer_Data.Image_Buffer_List[i] = NULL;
	}
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
	sensor_width = CCD_Setup_Get_Sensor_Width();
	sensor_height = CCD_Setup_Get_Sensor_Height();
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Buffer_Initialise: Binned 1 image size %d bytes vs "
			       "sensor size (%d x %d ).",Buffer_Data.Image_Size_Bytes,sensor_width,sensor_height);
#endif /* LOGGING */
	/* allocate buffers */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Buffer_Initialise: Allocating %d buffers of %d bytes size.",
			       BUFFER_COUNT,Buffer_Data.Image_Size_Bytes);
#endif /* LOGGING */
	for(i=0; i < BUFFER_COUNT; i++)
	{
		Buffer_Data.Image_Buffer_List[i] = (int *)malloc(Buffer_Data.Image_Size_Bytes* sizeof(int));
		if(Buffer_Data.Image_Buffer_List[i] == NULL)
		{
			Buffer_Error_Number = 2;
			sprintf(Buffer_Error_String,"CCD_Buffer_Initialise: Allocating %d bytes for "
				"buffer %d of %d failed.",Buffer_Data.Image_Size_Bytes,i,BUFFER_COUNT);
			return FALSE;
		}
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"CCD_Buffer_Initialise: Buffer %d = %p.",
				       i,Buffer_Data.Image_Buffer_List[i]);
#endif /* LOGGING */
	}/* end for */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Buffer_Initialise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Add a sequence of image buffers to the PCO library queue.
 * @param image_count The number of images we expect to acquire, we add this number of buffers to the PCO queue.
 * @see ccd_general.html#CCD_General_Log_Format
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #BUFFER_COUNT
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see #Buffer_Data
 * @see ccd_command.html#CCD_Command_Queue_Buffer
 * @see ccd_general.html#CCD_General_Log_Format
 * @see ccd_setup.html#CCD_Setup_Get_Image_Size_Bytes
 */
int CCD_Buffer_Queue_Images(int image_count)
{
	int i,buffer_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Buffer_Queue_Images: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Buffer_Queue_Images: Queueing %d image buffers.",image_count);
#endif /* LOGGING */
	for(i = 0; i < image_count; i++)
	{
		/* calculate image buffer list index */
		buffer_index = i%BUFFER_COUNT;
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,
				       "CCD_Buffer_Queue_Images: Using image buffer list index %d for image %d.",
				       buffer_index,i);
#endif /* LOGGING */
		/* Queue the buffer. Here we use CCD_Setup_Get_Image_Size_Bytes to get the binned image size,
		** as otherwise CCD_Command_Queue_Buffer returns INVALID_SIZE. The actual buffer
		** is allocated to the size Buffer_Data.Image_Size_Bytes, which was computed for binning 1 */
		/* diddly TODO
		if(!CCD_Command_Queue_Buffer(Buffer_Data.Image_Buffer_List[buffer_index],
					     CCD_Setup_Get_Image_Size_Bytes()))
		{
			Buffer_Error_Number = 3;
			sprintf(Buffer_Error_String,
				"CCD_Buffer_Initialise: Failed to queue image buffer list index %d (%p, size %d/%d) "
				"for image %d.",buffer_index,Buffer_Data.Image_Buffer_List[buffer_index],
				CCD_Setup_Get_Image_Size_Bytes(),Buffer_Data.Image_Size_Bytes,i);
			return FALSE;
		}
		*/
	}/* end for */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Buffer_Queue_Images: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Free previously allocated buffers.
 * <ul>
 * <li>We loop over BUFFER_COUNT. If each Buffer_Data.Image_Buffer_List pointer is non-NULL, we free the 
 *     allocated data and set the pointer to NULL.
 * </ul>
 * @see #Buffer_Data
 * @see #BUFFER_COUNT
 */
int CCD_Buffer_Free(void)
{
	int i;

	for(i=0; i < BUFFER_COUNT; i++)
	{
		if(Buffer_Data.Image_Buffer_List[i] != NULL)
			free(Buffer_Data.Image_Buffer_List[i]);
		Buffer_Data.Image_Buffer_List[i] = NULL;
	}
	return TRUE;
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
