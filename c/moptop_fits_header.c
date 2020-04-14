/* moptop_fits_header.c
*/
/**
 * Moptop FITS header handling.
 * @author $Author$
 * @version $Revision$
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "log_udp.h"

#include "ccd_general.h"
#include "ccd_fits_header.h"

#include "moptop_fits_header.h"
#include "moptop_general.h"
#include "ngat_astro.h"
#include "ngat_astro_mjd.h"

/* external variables */


/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id$";


/* internal routines */
/* ------------------------------------------------------------------
** External functions 
** ------------------------------------------------------------------ */
/**
 * Initialise the FITS header data.
 * @see #Fits_Header_List
 * @return The routine returns TRUE on success, and FALSE on failure. 
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Initialise
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 */
int Moptop_Fits_Header_Initialise(void)
{
	if(!CCD_Fits_Header_Initialise())
	{
		Moptop_General_Error_Number = 400;
		sprintf(Moptop_General_Error_String,"Moptop_Fits_Header_Initialise:"
			"Failed to Initialise FITS headers.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to add a keyword with a string value to the Fits_Header data structure.
 * @param keyword The keyword string.
 * @param value The value string.
 * @param comment The comment string. This parameter can also be NULL.
 * @return The routine returns TRUE on success, and FALSE on failure.
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Add_String
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 */
int Moptop_Fits_Header_String_Add(char *keyword,char *value, char *comment)
{
	if(!CCD_Fits_Header_Add_String(keyword,value,comment))
	{
		Moptop_General_Error_Number = 401;
		sprintf(Moptop_General_Error_String,"Moptop_Fits_Header_String_Add:"
			"Failed to add string to FITS headers.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to add a keyword with a integer value to the Fits_Header data structure.
 * @param keyword The keyword string.
 * @param value The value, an integer.
 * @param comment The comment string. This parameter can also be NULL.
 * @return The routine returns TRUE on success, and FALSE on failure.
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Add_Int
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 */
int Moptop_Fits_Header_Integer_Add(char *keyword,int value, char *comment)
{
	if(!CCD_Fits_Header_Add_Int(keyword,value,comment))
	{
		Moptop_General_Error_Number = 402;
		sprintf(Moptop_General_Error_String,"Moptop_Fits_Header_Integer_Add:"
			"Failed to add integer to FITS headers.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to add a keyword with a float (or double) value to the Fits_Header data structure.
 * @param keyword The keyword string.
 * @param value The value, a double.
 * @param comment The comment string. This parameter can also be NULL.
 * @return The routine returns TRUE on success, and FALSE on failure.
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Add_Float
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 */
int Moptop_Fits_Header_Float_Add(char *keyword,double value, char *comment)
{
	if(!CCD_Fits_Header_Add_Float(keyword,value,comment))
	{
		Moptop_General_Error_Number = 403;
		sprintf(Moptop_General_Error_String,"Moptop_Fits_Header_Float_Add:"
			"Failed to add float to FITS headers.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to add a keyword with a boolean value to the Fits_Header data structure.
 * @param keyword The keyword string.
 * @param value The value, an integer with value 0 (FALSE) or 1 (TRUE).
 * @param comment The comment string. This parameter can also be NULL.
 * @return The routine returns TRUE on success, and FALSE on failure.
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Add_Logical
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 */
int Moptop_Fits_Header_Logical_Add(char *keyword,int value, char *comment)
{
	if(!CCD_Fits_Header_Add_Logical(keyword,value,comment))
	{
		Moptop_General_Error_Number = 404;
		sprintf(Moptop_General_Error_String,"Moptop_Fits_Logical_Float_Add:"
			"Failed to add logical to FITS headers.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to delete the FITS header with the specified keyword from the Fits_Header data structure.
 * @param keyword The keyword string.
 * @return The routine returns TRUE on success, and FALSE on failure.
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Delete
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 */
int Moptop_Fits_Header_Delete(char *keyword)
{
	if(!CCD_Fits_Header_Delete(keyword))
	{
		Moptop_General_Error_Number = 405;
		sprintf(Moptop_General_Error_String,"Moptop_Fits_Header_Delete:"
			"Failed to delete FITS header with keyword %s.",keyword);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to clear the FITS headers from the Fits_Header data structure.
 * @return The routine returns TRUE on success, and FALSE on failure.
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Clear
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 */
int Moptop_Fits_Header_Clear(void)
{
	if(!CCD_Fits_Header_Clear())
	{
		Moptop_General_Error_Number = 406;
		sprintf(Moptop_General_Error_String,"Moptop_Fits_Header_Clear:Failed to clear FITS headers.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to convert a timespec structure to a DATE sytle string to put into a FITS header.
 * This uses gmtime_r and strftime to format the string. The resultant string is of the form:
 * <b>CCYY-MM-DD</b>, which is equivalent to %Y-%m-%d passed to strftime.
 * We now use gmtime_r, as there is a possibility of some status commands calling gmtime whilst
 * FITS headers are being formatted with gmtime, which can cause corrupted FITS headers (Fault #2096).
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	12 characters long.
 */
void Moptop_Fits_Header_TimeSpec_To_Date_String(struct timespec time,char *time_string)
{
	struct tm tm_time;

	gmtime_r(&(time.tv_sec),&tm_time);
	strftime(time_string,12,"%Y-%m-%d",&tm_time);
}

/**
 * Routine to convert a timespec structure to a DATE-OBS sytle string to put into a FITS header.
 * This uses gmtime_r and strftime to format most of the string, and tags the milliseconds on the end.
 * The resultant form of the string is <b>CCYY-MM-DDTHH:MM:SS.sss</b>.
 * We now use gmtime_r, as there is a possibility of some status commands calling gmtime whilst
 * FITS headers are being formatted with gmtime, which can cause corrupted FITS headers (Fault #2096).
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	24 characters long.
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_MILLISECOND_NS
 */
void Moptop_Fits_Header_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string)
{
	struct tm tm_time;
	char buff[32];
	int milliseconds;

	gmtime_r(&(time.tv_sec),&tm_time);
	strftime(buff,32,"%Y-%m-%dT%H:%M:%S.",&tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)MOPTOP_GENERAL_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

/**
 * Routine to convert a timespec structure to a UTSTART sytle string to put into a FITS header.
 * This uses gmtime_r and strftime to format most of the string, and tags the milliseconds on the end.
 * We now use gmtime_r, as there is a possibility of some status commands calling gmtime whilst
 * FITS headers are being formatted with gmtime, which can cause corrupted FITS headers (Fault #2096).
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	14 characters long.
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_MILLISECOND_NS
 */
void Moptop_Fits_Header_TimeSpec_To_UtStart_String(struct timespec time,char *time_string)
{
	struct tm tm_time;
	char buff[16];
	int milliseconds;

	gmtime_r(&(time.tv_sec),&tm_time);
	strftime(buff,16,"%H:%M:%S.",&tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)MOPTOP_GENERAL_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

/**
 * Routine to convert a timespec structure to a Modified Julian Date (decimal days) to put into a FITS header.
 * <p>If NGATASTRO is defined, this uses NGAT_Astro_Timespec_To_MJD to get the MJD.
 * <p>If NGATASTRO is not defined at compile time, this routine should throw an error
 * when compiling.
 * <p>This routine is still wrong for last second of the leap day, as gmtime will return 1st second of the next day.
 * Also note the passed in leap_second_correction should change at midnight, when the leap second occurs.
 * None of this should really matter, 1 second will not affect the MJD for several decimal places.
 * We now use gmtime_r, as there is a possibility of some status commands calling gmtime whilst
 * FITS headers are being formatted with gmtime, which can cause corrupted FITS headers (Fault #2096).
 * @param time The time to convert.
 * @param leap_second_correction A number representing whether a leap second will occur. This is normally zero,
 * 	which means no leap second will occur. It can be 1, which means the last minute of the day has 61 seconds,
 *	i.e. there are 86401 seconds in the day. It can be -1,which means the last minute of the day has 59 seconds,
 *	i.e. there are 86399 seconds in the day.
 * @param mjd The address of a double to store the calculated MJD.
 * @return The routine returns TRUE if it succeeded, FALSE if it fails. 
 *         slaCldj and NGAT_Astro_Timespec_To_MJD can fail.
 */
int Moptop_Fits_Header_TimeSpec_To_Mjd(struct timespec time,int leap_second_correction,double *mjd)
{
	int retval;

	retval = NGAT_Astro_Timespec_To_MJD(time,leap_second_correction,mjd);
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 407;
		sprintf(Moptop_General_Error_String,
			"Moptop_Fits_Header_TimeSpec_To_Mjd:NGAT_Astro_Timespec_To_MJD failed.\n");
		/* concatenate NGAT Astro library error onto Moptop_General_Error_String */
		NGAT_Astro_Error_String(Moptop_General_Error_String+strlen(Moptop_General_Error_String));
		return FALSE;
	}
	return TRUE;
}
