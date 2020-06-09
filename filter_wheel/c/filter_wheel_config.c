/* filter_wheel_config.c
** Starlight Express filter wheel config routines
*/
/**
 * Starlight Express filter wheel config routines. This software loads a filter name<-> filter wheel position
 * mapping from a config file, and provides routines to go between the filter name<-> filter wheel position mapping.
 * @author Chris Mottram
 * @version $Revision$
 */
#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "filter_wheel_general.h"
#include "filter_wheel_command.h"
#include "filter_wheel_config.h"

/**
 * The maximum length of filter wheel names read by this module.
 */
#define CONFIG_NAME_STRING_LENGTH   (32)

/* data types/structures */
/**
 * Data type holding local data to filter_wheel_config. This consists of the following:
 * <dl>
 * <dt>Position</dt> <dd>The position of the filter in the wheel.</dd>
 * <dt>Name</dt> <dd>The filter name (which type) of the filter, of length CONFIG_NAME_STRING_LENGTH.</dd>
 * <dt>Id</dt> <dd>The filter Id (which physical piece of glass) of the filter, 
 *                 of length CONFIG_NAME_STRING_LENGTH.</dd>
 * </dl>
 * @see #CONFIG_NAME_STRING_LENGTH
 */
struct Config_Struct
{
	int Position;
	char Name[CONFIG_NAME_STRING_LENGTH];
	char Id[CONFIG_NAME_STRING_LENGTH];
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * An array of Config_Structs that contains local data for this module. The array is of length 
 * FILTER_WHEEL_COMMAND_FILTER_COUNT+1. This enables us to hold data for each filter indexed by position
 * (with the '0' position being empty), as the actual physical positions in the wheel are numbered 
 * 1..FILTER_WHEEL_COMMAND_FILTER_COUNT.
 * @see #Config_Struct
 */
static struct Config_Struct Config_Data[FILTER_WHEEL_COMMAND_FILTER_COUNT+1];

/**
 * Variable holding error code of last operation performed.
 */
static int Config_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see filter_wheel_general.html#FILTER_WHEEL_GENERAL_ERROR_STRING_LENGTH
 */
static char Config_Error_String[FILTER_WHEEL_GENERAL_ERROR_STRING_LENGTH] = "";

/* =======================================
**  external functions 
** ======================================= */
/**
 * Initialise the filter wheel configuration. This loads the Config_Data array with
 * the values previously loaded from a configuration file and stored in a eSTAR_Config_Properties_t.
 * @param Config_Properties The instance of eSTAR_Config_Properties_t to laod the configuration data from.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Config_Data
 * @see #Config_Error_Number
 * @see #Config_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Log
 * @see filter_wheel_general.html#Filter_Wheel_General_Log_Format
 * @see filter_wheel_command.html#FILTER_WHEEL_COMMAND_FILTER_COUNT
 * @see ../../../estar/config/estar_config.html#eSTAR_Config_Properties_t
 * @see ../../../estar/config/estar_config.html#eSTAR_Config_Get_String
 */
int Filter_Wheel_Config_Initialise(eSTAR_Config_Properties_t Config_Properties)
{
	char keyword[32];
	char *value = NULL;
	int i,retval;

	Config_Error_Number = 0;
#if LOGGING > 0
	Filter_Wheel_General_Log(LOG_VERBOSITY_VERBOSE,"Filter_Wheel_Config_Initialise: Started.");
#endif /* LOGGING */
	/* blank the 0 index config data - the valid positions are 1 to 5 (FILTER_WHEEL_COMMAND_FILTER_COUNT) */
	Config_Data[0].Position = 0;
	strcpy(Config_Data[0].Name,"None");
	strcpy(Config_Data[0].Id,"None");
	/* loop over the valid positions 1 to 5 (FILTER_WHEEL_COMMAND_FILTER_COUNT) */
	for(i = 1; i <= FILTER_WHEEL_COMMAND_FILTER_COUNT; i++)
	{
		/* position */
		Config_Data[i].Position = i;
		/* name */
		sprintf(keyword,"filter_wheel.filter.name.%d",i);
		retval = eSTAR_Config_Get_String(&Config_Properties,keyword,&value);
		if(retval == FALSE)
		{
			Config_Error_Number = 1;
			sprintf(Config_Error_String,
				"Filter_Wheel_Config_Initialise: failed to get value for keyword '%s'.",keyword);
			return FALSE;
		}
		if((strlen(value)+1) > CONFIG_NAME_STRING_LENGTH)
		{
			Config_Error_Number = 2;
			sprintf(Config_Error_String,
				"Filter_Wheel_Config_Initialise: keyword '%s' value '%s' is too long (%lu vs %d).",
				keyword,value,(strlen(value)+1),CONFIG_NAME_STRING_LENGTH);
			return FALSE;
		}
		strcpy(Config_Data[i].Name,value);
		if(value != NULL)
			free(value);
		value = NULL;
		/* id */
		sprintf(keyword,"filter_wheel.filter.id.%d",i);
		retval = eSTAR_Config_Get_String(&Config_Properties,keyword,&value);
		if(retval == FALSE)
		{
			Config_Error_Number = 6;
			sprintf(Config_Error_String,
				"Filter_Wheel_Config_Initialise: failed to get value for keyword '%s'.",keyword);
			return FALSE;
		}
		if((strlen(value)+1) > CONFIG_NAME_STRING_LENGTH)
		{
			Config_Error_Number = 7;
			sprintf(Config_Error_String,
				"Filter_Wheel_Config_Initialise: keyword '%s' value '%s' is too long (%lu vs %d).",
				keyword,value,(strlen(value)+1),CONFIG_NAME_STRING_LENGTH);
			return FALSE;
		}
		strcpy(Config_Data[i].Id,value);
		if(value != NULL)
			free(value);
		value = NULL;
#if LOGGING > 5
		Filter_Wheel_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
						"Filter_Wheel_Config_Initialise: Config Data index %d for Position %d "
						"with name '%s' and id '%s'.",
						i,Config_Data[i].Position,Config_Data[i].Name,Config_Data[i].Id);
#endif /* LOGGING */
	}/* end for */
#if LOGGING > 0
	Filter_Wheel_General_Log(LOG_VERBOSITY_VERBOSE,"Filter_Wheel_Config_Initialise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the physical position associated with the specified filter name.
 * @param filter_name The name of the string to get the position for.
 * @param position The address of an integer to store the retrieved physical filter position 
 *       (1 to 5 (FILTER_WHEEL_COMMAND_FILTER_COUNT)).
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Config_Data
 * @see #Config_Error_Number
 * @see #Config_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Log_Format
 */
int Filter_Wheel_Config_Name_To_Position(char* filter_name,int *position)
{
	int found,index;

	Config_Error_Number = 0;
	if(filter_name == NULL)
	{
		Config_Error_Number = 12;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Name_To_Position: filter_name is NULL.");
		return FALSE;
	}
	if(position == NULL)
	{
		Config_Error_Number = 3;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Name_To_Position: position is NULL.");
		return FALSE;
	}
#if LOGGING > 5
	Filter_Wheel_General_Log_Format(LOG_VERBOSITY_VERBOSE,
					"Filter_Wheel_Config_Name_To_Position: Looking for Filter '%s'.",filter_name);
#endif /* LOGGING */
	found = FALSE;
	index = 0;
	while((found == FALSE)&&(index <= FILTER_WHEEL_COMMAND_FILTER_COUNT))
	{
#if LOGGING > 5
		Filter_Wheel_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
		     "Filter_Wheel_Config_Name_To_Position: Index %d: Comparing Filter '%s' to '%s'.",
						index,filter_name,Config_Data[index].Name);
#endif /* LOGGING */
		if(strcmp(filter_name,Config_Data[index].Name) == 0)
		{
#if LOGGING > 5
			Filter_Wheel_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			     "Filter_Wheel_Config_Name_To_Position: Found match at Index %d, Position %d.",
						index,Config_Data[index].Position);
#endif /* LOGGING */
			(*position) = Config_Data[index].Position;
			found  = TRUE;
		}
		index++;
	}/* end while */
	if(found == FALSE)
	{
		Config_Error_Number = 4;
		sprintf(Config_Error_String,
			"Filter_Wheel_Config_Name_To_Position: Failed to find filter name '%s' in list of length %d.",
			filter_name,FILTER_WHEEL_COMMAND_FILTER_COUNT);
		return FALSE;
	}
#if LOGGING > 5
	Filter_Wheel_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
					"Filter_Wheel_Config_Name_To_Position: Filter '%s' has position %d.",
					filter_name,(*position));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the name associated with the physical position.
 * @param position The physical filter position to retrieve (1 to 5 (FILTER_WHEEL_COMMAND_FILTER_COUNT)).
 * @param filter_name The string to copy the name of the filter at this position into. The string should be at 
 *        least CONFIG_NAME_STRING_LENGTH long.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #CONFIG_NAME_STRING_LENGTH
 * @see #Config_Data
 * @see #Config_Error_Number
 * @see #Config_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Log_Format
 */
int Filter_Wheel_Config_Position_To_Name(int position,char* filter_name)
{
	Config_Error_Number = 0;
	if(filter_name == NULL)
	{
		Config_Error_Number = 13;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Position_To_Name: filter_name is NULL.");
		return FALSE;
	}
	if((position < 1) || (position > FILTER_WHEEL_COMMAND_FILTER_COUNT))
	{
		Config_Error_Number = 5;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Position_To_Name: Position %d out of range (1..%d).",
			position,FILTER_WHEEL_COMMAND_FILTER_COUNT);
		return FALSE;
	}
	strcpy(filter_name,Config_Data[position].Name);
#if LOGGING > 5
	Filter_Wheel_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
					"Filter_Wheel_Config_Position_To_Name: Position %d has filter name '%s'.",
					position,filter_name);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the filter id associated with the specified filter name.
 * @param filter_name The name of the string to get the position for.
 * @param id A string to store the id of the filter with the specified name. 
 *           The string must be at least characters long.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Config_Data
 * @see #Config_Error_Number
 * @see #Config_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Log_Format
 */
int Filter_Wheel_Config_Name_To_Id(char* filter_name,char *id)
{
	int found,index;

	Config_Error_Number = 0;
	if(filter_name == NULL)
	{
		Config_Error_Number = 9;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Name_To_Id: filter_name is NULL.");
		return FALSE;
	}
	if(id == NULL)
	{
		Config_Error_Number = 10;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Name_To_Id: id is NULL.");
		return FALSE;
	}
#if LOGGING > 5
	Filter_Wheel_General_Log_Format(LOG_VERBOSITY_VERBOSE,
					"Filter_Wheel_Config_Name_To_Id: Looking for Filter '%s'.",filter_name);
#endif /* LOGGING */
	found = FALSE;
	index = 0;
	while((found == FALSE)&&(index <= FILTER_WHEEL_COMMAND_FILTER_COUNT))
	{
#if LOGGING > 5
		Filter_Wheel_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
		     "Filter_Wheel_Config_Name_To_Id: Index %d: Comparing Filter '%s' to '%s'.",
						index,filter_name,Config_Data[index].Name);
#endif /* LOGGING */
		if(strcmp(filter_name,Config_Data[index].Name) == 0)
		{
#if LOGGING > 5
			Filter_Wheel_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			     "Filter_Wheel_Config_Name_To_Id: Found match at Index %d, Position %d, id '%s'.",
							index,Config_Data[index].Position,Config_Data[index].Id);
#endif /* LOGGING */
			strcpy(id,Config_Data[index].Id);
			found  = TRUE;
		}
		index++;
	}/* end while */
	if(found == FALSE)
	{
		Config_Error_Number = 11;
		sprintf(Config_Error_String,
			"Filter_Wheel_Config_Name_To_Id: Failed to find filter name '%s' in list of length %d.",
			filter_name,FILTER_WHEEL_COMMAND_FILTER_COUNT);
		return FALSE;
	}
#if LOGGING > 5
	Filter_Wheel_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
					"Filter_Wheel_Config_Name_To_Id: Filter '%s' has id '%s'.",filter_name,id);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the id associated with the physical position.
 * @param position The physical filter position to retrieve (1 to 5 (FILTER_WHEEL_COMMAND_FILTER_COUNT)).
 * @param id The string to copy the id of the filter at this position into. The string should be at 
 *        least CONFIG_NAME_STRING_LENGTH long.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #CONFIG_NAME_STRING_LENGTH
 * @see #Config_Data
 * @see #Config_Error_Number
 * @see #Config_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Log_Format
 */
int Filter_Wheel_Config_Position_To_Id(int position,char* id)
{
	Config_Error_Number = 0;
	if(id == NULL)
	{
		Config_Error_Number = 14;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Position_To_Id: id is NULL.");
		return FALSE;
	}
	if((position < 1) || (position > FILTER_WHEEL_COMMAND_FILTER_COUNT))
	{
		Config_Error_Number = 8;
		sprintf(Config_Error_String,"Filter_Wheel_Config_Position_To_Id: Position %d out of range (1..%d).",
			position,FILTER_WHEEL_COMMAND_FILTER_COUNT);
		return FALSE;
	}
	strcpy(id,Config_Data[position].Id);
#if LOGGING > 5
	Filter_Wheel_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
					"Filter_Wheel_Config_Position_To_Id: Position %d has filter id '%s'.",
					position,id);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Config_Error_Number
 */
int Filter_Wheel_Config_Get_Error_Number(void)
{
	return Config_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Config_Error_Number
 * @see #Config_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Get_Current_Time_String
 */
void Filter_Wheel_Config_Error(void)
{
	char time_string[32];

	Filter_Wheel_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Config_Error_Number == 0)
		sprintf(Config_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Filter_Wheel_Config:Error(%d) : %s\n",time_string,
		Config_Error_Number,Config_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Config_Error_Number
 * @see #Config_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Get_Current_Time_String
 */
void Filter_Wheel_Config_Error_String(char *error_string)
{
	char time_string[32];

	Filter_Wheel_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Config_Error_Number == 0)
		sprintf(Config_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Filter_Wheel_Config:Error(%d) : %s\n",time_string,
		Config_Error_Number,Config_Error_String);
}

