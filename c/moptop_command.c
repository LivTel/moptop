/* moptop_command.c
** Moptop command routines
*/
/**
 * Command routines for Moptop.
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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
/**
 * Add more fields to struct tm (tm_tm_zone).
 */
#define _BSD_SOURCE
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

#include "ccd_exposure.h"
#include "ccd_fits_filename.h"
#include "ccd_fits_header.h"
#include "ccd_general.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

#include "command_server.h"

#include "moptop_fits_header.h"
#include "moptop_general.h"
#include "moptop_server.h"

/* hash defines */
/**
 * Timezone offset for 1 hour.
 */
#define TIMEZONE_OFFSET_HOUR (3600)
/**
 * Timezone offset for BST.
 */
#define TIMEZONE_OFFSET_BST  (TIMEZONE_OFFSET_HOUR)
/**
 * Timezone offset for HST (roughly!).
 */
#define TIMEZONE_OFFSET_HST  (-10*TIMEZONE_OFFSET_HOUR)

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";

/* internal functions */
static int Command_Parse_Date(char *time_string,int *time_secs);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Handle a command of the form: "abort".
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Abort
 */
int Moptop_Command_Abort(char *command_string,char **reply_string)
{
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Abort",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif

	/* diddly not implemented */

	if(!Moptop_General_Add_String(reply_string,"1 Abort not implemented yet."))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Abort",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a command of the form: "config <filter> <bin> <trigger_mode>".
 * <ul>
 * <li>
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Dimensions
 */
int Moptop_Command_Config(char *command_string,char **reply_string)
{
	int retval,bin;
	char filter_string[9];
	char trigger_mode_string[9];

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"config %8s %d %8s",filter_string,&bin,trigger_mode_string);
	if(retval != 3)
	{
		Moptop_General_Error_Number = 501;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Config:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse config command."))
			return FALSE;
		return TRUE;
	}

	/* diddly not implemented */

	if(!Moptop_General_Add_String(reply_string,"1 Config not implemented yet."))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Implementation of FITS Header commands.
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_fits_header.html#Moptop_Fits_Header_Logical_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Float_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Integer_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_String_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Clear
 * @see moptop_fits_header.html#Moptop_Fits_Header_Delete
 */
int Moptop_Command_Fits_Header(char *command_string,char **reply_string)
{
	char operation_string[8];
	char keyword_string[13];
	char type_string[8];
	char value_string[80];
	int retval,command_string_index,ivalue,value_index;
	double dvalue;

	/* parse command to retrieve operation*/
	retval = sscanf(command_string,"fitsheader %6s %n",operation_string,&command_string_index);
	if((retval != 1)&&(retval != 2)) /* sscanf isn't sure whether %n increments returned value! */
	{
		Moptop_General_Error_Number = 517;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
			"Failed to parse command %s (%d).",command_string,retval);
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader command."))
			return FALSE;
		return TRUE;
	}
	/* do operation */
	if(strncmp(operation_string,"add",3) == 0)
	{
		retval = sscanf(command_string+command_string_index,"%12s %7s %n",keyword_string,type_string,
				&value_index);
		if((retval != 3)&&(retval != 2)) /* %n may or may not increment retval*/
		{
			Moptop_General_Error_Number = 518;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
				"Failed to parse add command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (add command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader add command."))
				return FALSE;
			return TRUE;
		}
		strncpy(value_string,command_string+command_string_index+value_index,79);
		value_string[79] = '\0';
		if(strncmp(type_string,"boolean",7)==0)
		{
			/* parse value */
			if(strncmp(value_string,"true",4) == 0)
				ivalue = TRUE;
			else if(strncmp(value_string,"false",5) == 0)
				ivalue = FALSE;
			else
			{
				Moptop_General_Error_Number = 519;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
					"Add boolean command had unknown value %s.",value_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add boolean command had unknown value %s.",value_string);
#endif
				if(!Moptop_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add boolean command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Moptop_Fits_Header_Logical_Add(keyword_string,ivalue,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add boolean to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add boolean fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"float",5)==0)
		{
			/* parse value */
			retval = sscanf(value_string,"%lf",&dvalue);
			if(retval != 1)
			{
				Moptop_General_Error_Number = 520;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
					"Add float command had unknown value %s.",value_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add float command had unknown value %s.",value_string);
#endif
				if(!Moptop_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add float command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Moptop_Fits_Header_Float_Add(keyword_string,dvalue,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add float to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add float fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"integer",7)==0)
		{
			/* parse value */
			retval = sscanf(value_string,"%d",&ivalue);
			if(retval != 1)
			{
				Moptop_General_Error_Number = 521;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
					"Add integer command had unknown value %s.",value_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add integer command had unknown value %s.",value_string);
#endif
				if(!Moptop_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add integer command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Moptop_Fits_Header_Integer_Add(keyword_string,ivalue,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add integer to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add integer fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"string",6)==0)
		{
			/* do operation */
			if(!Moptop_Fits_Header_String_Add(keyword_string,value_string,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add string to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add string fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else
		{
			Moptop_General_Error_Number = 522;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
				"Add command had unknown type %s.",type_string);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
				       LOG_VERBOSITY_TERSE,"COMMAND","Add command had unknown type %s.",type_string);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader add command type."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(operation_string,"delete",6) == 0)
	{
		retval = sscanf(command_string+command_string_index,"%12s",keyword_string);
		if(retval != 1)
		{
			Moptop_General_Error_Number = 523;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
				"Failed to parse delete command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (delete command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader delete command."))
				return FALSE;
			return TRUE;
		}
		/* do delete */
		if(!Moptop_Fits_Header_Delete(keyword_string))
		{
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to delete FITS header with keyword '%s'.",keyword_string);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to delete fits header."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(operation_string,"clear",5) == 0)
	{
		/* do clear */
		if(!Moptop_Fits_Header_Clear())
		{
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","Failed to clear FITS header.");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to clear fits header."))
				return FALSE;
			return TRUE;
		}
	}
	else
	{
		Moptop_General_Error_Number = 524;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
			"Unknown operation %s:Failed to parse command %s.",operation_string,command_string);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
					  LOG_VERBOSITY_TERSE,
					  "COMMAND","Unknown operation %s:Failed to parse command %s.",
					  operation_string,command_string);
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader command: Unknown operation."))
			return FALSE;
		return TRUE;
	}
	if(!Moptop_General_Add_String(reply_string,"0 FITS Header command succeeded."))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a command of the form: "multrun <length> <count> <standard>".
 * <ul>
 * <li>The multrun command is parsed to get the exposure length, count and standard (true|false) values.
 * <li>The OBSTYPE FITS header is set using Moptop_Fits_Header_String_Add.
 * <li>
 * <li>The reply string is constructed of the form "0 <filename count> <multrun number> <last FITS filename>".
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_fits_header.html#Moptop_Fits_Header_String_Add
 * @see moptop_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_List_Free
 */
int Moptop_Command_Multrun(char *command_string,char **reply_string)
{
	struct timespec start_time = {0L,0L};
	char **filename_list = NULL;
	char standard_string[8];
	char count_string[16];
	int i,retval,exposure_length,exposure_count,filename_count,do_standard,multrun_number;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"multrun %d %d %7s",&exposure_length,&exposure_count,standard_string);
	if(retval != 3)
	{
		Moptop_General_Error_Number = 505;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Multrun:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse multrun command."))
			return FALSE;
		return TRUE;
	}
	/* parse standard string */
	if(strcmp(standard_string,"true") == 0)
		do_standard = TRUE;
	else if(strcmp(standard_string,"false") == 0)
		do_standard = FALSE;
	else
	{
		Moptop_General_Error_Number = 539;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Multrun:Illegal standard value '%s'.",
			standard_string);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Moptop_General_Add_String(reply_string,"1 Multrun failed:Illegal standard value."))
			return FALSE;
		return TRUE;
	}
	/* OBSTYPE FITS header */
	if(do_standard)
		retval = Moptop_Fits_Header_String_Add("OBSTYPE","STANDARD",NULL);
	else
		retval = Moptop_Fits_Header_String_Add("OBSTYPE","EXPOSE",NULL);
	if(retval == FALSE)
	{
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Moptop_General_Add_String(reply_string,"1 Multrun failed:Failed to set OBSTYPE."))
			return FALSE;
		return TRUE;
	}
	/* do multrun */

	/* diddly not implemented yet */

	/* success */
	if(!Moptop_General_Add_String(reply_string,"0 "))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add number of FITS images */
	sprintf(count_string,"%d ",filename_count);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* get multrun number */
	/* either the actual multrun number if all cameras have the same, else -1 */
	multrun_number = CCD_Fits_Filename_Multrun_Get();
	sprintf(count_string,"%d ",multrun_number);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add last filename */
	if(filename_count > 0)
	{
		if(!Moptop_General_Add_String(reply_string,filename_list[filename_count-1]))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	else
	{
		if(!Moptop_General_Add_String(reply_string,"none"))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	/* log filenames returned */
	for(i=0; i < filename_count; i++)
	{
#if MOPTOP_DEBUG > 8
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Multrun",
					  LOG_VERBOSITY_VERY_VERBOSE,"COMMAND","Filename %d : %s",i,filename_list[i]);
#endif
	}
	if(!CCD_Fits_Filename_List_Free(&filename_list,&filename_count))
	{
		Moptop_General_Error_Number = 510;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Multrun:CCD_Fits_Filename_List_Free failed.");
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Moptop_General_Add_String(reply_string,"1 Multrun failed (freeing filename list)."))
			return FALSE;
		return TRUE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a status command. Possible forms: 
 * <ul>
 * <li>status temperature [get|status]
 * <li>status exposure [status|count|length|start_time|trigger_mode|elapsed_time]
 * <li>status exposure [accumulation|series|index|multrun|run|window]
 * <li>status [name|identification|fits_instrument_code]
 * </ul>
 * <ul>
 * <li>The status command is parsed to retrieve the subsystem (1st parameter).
 * <li>Based on the subsystem, further parsing occurs.
 * <li>The relevant status is retrieved, and a suitable reply constructed.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Status_To_String
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_EXPOSURE_TRIGGER_MODE
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Run_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Window_Get
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Get_Time_String
 */
int Moptop_Command_Status(char *command_string,char **reply_string)
{
	struct timespec status_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_status_time;
#endif
	char time_string[32];
	char return_string[128];
	char subsystem_string[32];
	char get_set_string[16];
	char key_string[64];
	char instrument_code;
	char *camera_name_string = NULL;
	int retval,command_string_index,ivalue,camera_index,serial_number;
	double temperature;
	
	/* parse command */
	retval = sscanf(command_string,"status %31s %n",subsystem_string,&command_string_index);
	if((retval != 1)&&(retval != 2)) /* sscanf isn't sure whether %n increments returned value! */
	{
		Moptop_General_Error_Number = 511;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse status command."))
			return FALSE;
		return TRUE;
	}
	/* initialise return string */
	strcpy(return_string,"0 ");
	/* parse subsystem */
	if(strncmp(subsystem_string,"exposure",8) == 0)
	{
		/*
		if(strncmp(command_string+command_string_index,"status",6)==0)
		{
			exposure_status = CCD_Multi_Exposure_Status_Get();
			strcat(return_string,CCD_Exposure_Status_To_String(exposure_status));
		}
		else if(strncmp(command_string+command_string_index,"count",5)==0)
		{
			ivalue = CCD_Multi_Exposure_Count_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"trigger_mode",12)==0)
		{
			trigger_mode = CCD_Multi_Exposure_Trigger_Mode_Get();
			if(trigger_mode == CCD_EXPOSURE_TRIGGER_MODE_EXTERNAL)
				strcat(return_string,"EXTERNAL");
			else if(trigger_mode == CCD_EXPOSURE_TRIGGER_MODE_INTERNAL)
				strcat(return_string,"INTERNAL");
			else
				strcat(return_string,"UNKNOWN");
		}
		else if(strncmp(command_string+command_string_index,"length",6)==0)
		{
			ivalue = CCD_Multi_Exposure_Exposure_Length_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"start_time",10)==0)
		{
			status_time = CCD_Multi_Exposure_Start_Time_Get();
			CCD_Global_Get_Time_String(status_time,time_string,31);
			sprintf(return_string+strlen(return_string),"%s",time_string);
		}
		else if(strncmp(command_string+command_string_index,"elapsed_time",12)==0)
		{
			ivalue = CCD_Multi_Exposure_Elapsed_Exposure_Time_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		*/
		/* these ones have a camera_index parameter */
		/* the camera_index is the ANDOR camera index not an index into a list of cameras controlled
		** by this C layer. See the Java GET_STATUS command for details. */
		/*
		else if(strncmp(command_string+command_string_index,"accumulation",12)==0)
		{
			retval = sscanf(command_string,"status exposure accumulation %d",&camera_index);
			if(retval != 1)
			{
				if(!Moptop_General_Add_String(reply_string,
				    "1 Failed to parse status exposure accumulation camera index."))
					return FALSE;
				return TRUE;
			}
			ivalue = CCD_Multi_Exposure_Accumulation_Get(camera_index);
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"series",6)==0)
		{
			retval = sscanf(command_string,"status exposure series %d",&camera_index);
			if(retval != 1)
			{
				if(!Moptop_General_Add_String(reply_string,
				    "1 Failed to parse status exposure series camera index."))
					return FALSE;
				return TRUE;
			}
			ivalue = CCD_Multi_Exposure_Series_Get(camera_index);
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"index",5)==0)
		{
			retval = sscanf(command_string,"status exposure index %d",&camera_index);
			if(retval != 1)
			{
				if(!Moptop_General_Add_String(reply_string,
				    "1 Failed to parse status exposure index camera index."))
					return FALSE;
				return TRUE;
			}
			ivalue = CCD_Multi_Exposure_Index_Get(camera_index);
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"multrun",7)==0)
		{
			retval = sscanf(command_string,"status exposure multrun %d",&camera_index);
			if(retval != 1)
			{
				if(!Moptop_General_Add_String(reply_string,
				    "1 Failed to parse status exposure multrun camera index."))
					return FALSE;
				return TRUE;
			}
			ivalue = CCD_Fits_Filename_Multrun_Get(camera_index);
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"run",3)==0)
		{
			retval = sscanf(command_string,"status exposure run %d",&camera_index);
			if(retval != 1)
			{
				if(!Moptop_General_Add_String(reply_string,
				    "1 Failed to parse status exposure run camera index."))
					return FALSE;
				return TRUE;
			}
			ivalue = CCD_Fits_Filename_Run_Get(camera_index);
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"window",6)==0)
		{
			retval = sscanf(command_string,"status exposure window %d",&camera_index);
			if(retval != 1)
			{
				if(!Moptop_General_Add_String(reply_string,
				    "1 Failed to parse status exposure window camera index."))
					return FALSE;
				return TRUE;
			}
			ivalue = CCD_Fits_Filename_Window_Get(camera_index);
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else
		{
			Moptop_General_Error_Number = 512;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse exposure command %s.",command_string+command_string_index);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND","Failed to parse exposure command %s.",
						  command_string+command_string_index);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse exposure status command."))
				return FALSE;
			return TRUE;
		}
		*/
	}
	else if(strncmp(subsystem_string,"fits_instrument_code",20) == 0)
	{
		sprintf(key_string,"file.fits.instrument_code");
		if(!Moptop_Config_Get_Character(key_string,&instrument_code))
		{
			Moptop_General_Error_Number = 546;
			sprintf(Moptop_General_Error_String,
			       "Moptop_Command_Status:Failed to get instrument code config.");
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (failed to get instrument code).");
#endif
			if(!Moptop_General_Add_String(reply_string,
						      "1 Failed to get instrument code."))
				return FALSE;
			return FALSE;
		}
		sprintf(return_string+strlen(return_string),"%c",instrument_code);
	}
	/*
	else if(strncmp(subsystem_string,"identification",14) == 0)
	{
		if(!CCD_Setup_Get_Cached_Camera_Identification(camera_index,head_model_name,&serial_number))
		{
			Moptop_General_Error_Number = 540;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to get camera identification.");
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
					   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get camera identification.");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to get camera identification."))
				return FALSE;
			return TRUE;
		}
		sprintf(return_string+strlen(return_string),"%s %d",head_model_name,serial_number);
	}
	*/
	/*
	else if(strncmp(subsystem_string,"name",4) == 0)
	{
		sprintf(key_string,"ccd.name");
		if(!Moptop_Config_Get_String(key_string,&camera_name_string))
		{
			Moptop_General_Error_Number = 548;
			sprintf(Moptop_General_Error_String,
				"Moptop_Startup_CCD:Failed to get camera name for Andor camera index %d.",
				camera_index);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (failed to get camera name).");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to get camera name."))
				return FALSE;
			return FALSE;
		}
		sprintf(return_string+strlen(return_string),"%s",camera_name_string);
		if(camera_name_string != NULL)
			free(camera_name_string);
	}
	*/
	else if(strncmp(subsystem_string,"temperature",11) == 0)
	{
		retval = sscanf(command_string,"status temperature %15s %d",get_set_string,&camera_index);
		if(retval != 2)
		{
			Moptop_General_Error_Number = 526;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,
						      "1 Failed to parse status temperature ."))
				return FALSE;
			return TRUE;
		}
		/* get exposure status */
		/*
		exposure_status = CCD_Multi_Exposure_Status_Get();
		*/
		/* if no exposure ongoing, get actual status */
		/*
		if(exposure_status == CCD_EXPOSURE_STATUS_NONE)
		{
			if(!CCD_Temperature_Get(camera_index,&temperature,&temperature_status))
			{
				Moptop_General_Error_Number = 513;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
					"Failed to get temperature.");
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
						   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get temperature.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to get temperature."))
					return FALSE;
				return TRUE;
			}
		*/
			/* set status_time to now */
		/*
#ifdef _POSIX_TIMERS
			clock_gettime(CLOCK_REALTIME,&status_time);
#else
			gettimeofday(&gtod_status_time,NULL);
			status_time.tv_sec = gtod_status_time.tv_sec;
			status_time.tv_nsec = gtod_status_time.tv_usec*MOPTOP_GLOBAL_ONE_MICROSECOND_NS;
#endif
		}
		*/
		/*else*/ /* get cached temperature status */
		/*
		{
			if(!CCD_Temperature_Get_Cached_Temperature(camera_index,&temperature,&temperature_status,
								   &status_time))
			{
				Moptop_General_Error_Number = 514;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
					"Failed to get cached temperature.");
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
						   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get cached temperature.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to get cached temperature."))
					return FALSE;
				return TRUE;
			}
			}*//* end if exposure_status is not NONE */
		/* check subcommand */
		/*
		if(strncmp(get_set_string,"get",3)==0)
		{
			CCD_General_Get_Time_String(status_time,time_string,31);
			sprintf(return_string+strlen(return_string),"%s %.2f",time_string,temperature);
		}
		else if(strncmp(get_set_string,"status",6)==0)
		{
			CCD_General_Get_Time_String(status_time,time_string,31);
			sprintf(return_string+strlen(return_string),"%s %s",time_string,
				CCD_Temperature_Status_To_String(temperature_status));
		}
		else
		{
			Moptop_General_Error_Number = 515;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse temperature command %s from %d.",command_string,command_string_index);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse temperature command %s from %d.",
						  command_string,command_string_index);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse temperature status command."))
				return FALSE;
			return TRUE;
		}
		*/
	}
	else
	{
		Moptop_General_Error_Number = 516;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
			"Unknown subsystem %s:Failed to parse command %s.",subsystem_string,command_string);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",LOG_VERBOSITY_TERSE,
					  "COMMAND","Unknown subsystem %s:Failed to parse command %s.",
					  subsystem_string,command_string);
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse status command."))
			return FALSE;
		return TRUE;
	}
	/* success */
	if(!Moptop_General_Add_String(reply_string,return_string))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Parse a date of the form "2007-05-03T07:38:48.099 UTC" into number of seconds since 1970 (unix time).
 * @param time_string The string.
 * @param time_secs The address of an integer to store the number of seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #TIMEZONE_OFFSET_BST
 * @see #TIMEZONE_OFFSET_HST
 */
static int Command_Parse_Date(char *time_string,int *time_secs)
{
	struct tm time_data;
	int year,month,day,hours,minutes,retval;
	double seconds;
	char timezone_string[16];
	time_t time_in_secs;

	/* check parameters */
	if(time_string == NULL)
	{
		Moptop_General_Error_Number = 528;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:time_string was NULL.");
		return FALSE;
	}
	if(time_secs == NULL)
	{
		Moptop_General_Error_Number = 529;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:time_secs was NULL.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 9
	Moptop_General_Log_Format("command","moptop_command.c","Command_Parse_Date",LOG_VERBOSITY_TERSE,
				  "COMMAND","Parsing date/time '%s'.",time_string);
#endif
	/* parse time_string into fields */
	strcpy(timezone_string,"UTC");
	retval = sscanf(time_string,"%d-%d-%d T %d:%d:%lf %15s",&year,&month,&day,
			&hours,&minutes,&seconds,timezone_string);
	if(retval < 6)
	{
		Moptop_General_Error_Number = 530;
		sprintf(Moptop_General_Error_String,
			"Command_Parse_Date:Failed to parse '%s', only parsed %d fields: year=%d,month=%d,day=%d,"
			"hour=%d,minute=%d,second=%.2f,timezone_string=%s.",time_string,retval,year,month,day,
			hours,minutes,seconds,timezone_string);
		return FALSE;
	}
#if MOPTOP_DEBUG > 9
	Moptop_General_Log_Format("command","moptop_command.c","Command_Parse_Date",LOG_VERBOSITY_TERSE,"COMMAND",
			    "Date/time '%s' has year=%d,month=%d,day=%d,hour=%d,minute=%d,seconds=%.2lf,timezone=%s.",
				  time_string,year,month,day,hours,minutes,seconds,timezone_string);
#endif
	/* construct tm */
	time_data.tm_year  = year-1900; /* years since 1900 */
	time_data.tm_mon = month-1; /* 0..11 */
	time_data.tm_mday  = day; /* 1..31 */
	time_data.tm_hour  = hours; /* 0..23 */
	time_data.tm_min   = minutes;
	time_data.tm_sec   = seconds;
	time_data.tm_wday  = 0;
	time_data.tm_yday  = 0;
	time_data.tm_isdst = 0;
	/* BSD extension stuff */
	/*
	time_data.tm_gmtoff = 0;
	time_data.tm_zone = strdup(timezone_string);
	*/
	/* create time in UTC */
	time_in_secs = mktime(&time_data);
	if(time_in_secs < 0)
	{
		Moptop_General_Error_Number = 532;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:mktime failed.",timezone_string);
		return FALSE;
	}
	(*time_secs) = (int)time_in_secs;
	if(strcmp(timezone_string,"UTC") == 0)
	{
		/* do nothing */
		(*time_secs) = (*time_secs);
	}
	else if(strcmp(timezone_string,"GMT") == 0)
	{
		/* do nothing */
		(*time_secs) = (*time_secs);
	}
	else if(strcmp(timezone_string,"BST") == 0)
	{
		(*time_secs) += TIMEZONE_OFFSET_BST;
	}
	else if(strcmp(timezone_string,"HST") == 0)
	{
		(*time_secs) += TIMEZONE_OFFSET_HST;
	}
	else
	{
		Moptop_General_Error_Number = 531;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:Unknown timezone '%s'.",timezone_string);
		return FALSE;
	}
	return TRUE;
}
