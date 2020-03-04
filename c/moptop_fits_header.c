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
