/* moptop_general.c
** Moptop general routines
*/
/**
 * General routines (logging, errror etc) for the moptop program.
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
/**
 * This hash define is needed before including string.h to give us the strdup prototype.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h> /* mutex */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_create.h"
#include "log_general.h"
#include "log_udp.h"

#include "command_server.h"

#include "ccd_general.h"

#include "moptop_general.h"

/* typedefs */
/**
 * typedef of a log handler function.
 */
typedef void (*Log_Handler_Function)(char *sub_system,char *source_filename,char *function,int level,char *category,
			    char *message);

/* hash defines */
/**
 * Length of some filenames
 */
#define MOPTOP_GENERAL_FILENAME_LENGTH      (256)
/**
 * Number of log handlers in the log handler list.
 */
#define LOG_HANDLER_LIST_COUNT                  (5)

/* external variables */
/**
 * Variable holding error code of last operation performed.
 */
int Moptop_General_Error_Number = 0;
/**
 * Internal variable holding description of the last error that occured.
 * @see #MOPTOP_GENERAL_ERROR_STRING_LENGTH
 */
char Moptop_General_Error_String[MOPTOP_GENERAL_ERROR_STRING_LENGTH] = "";

/* data types */
/**
 * Data type holding local data to moptop_general. This consists of the following:
 * <dl>
 * <dt>Log_Handler</dt> <dd>Function pointer to the routine that will log messages passed to it.</dd>
 * <dt>Log_Filter</dt> <dd>Function pointer to the routine that will filter log messages passed to it.
 * 		The funtion will return TRUE if the message should be logged, and FALSE if it shouldn't.</dd>
 * <dt>Log_Filter_Level</dt> <dd>A globally maintained log filter level. 
 * 		This is set using Moptop_General_Set_Log_Filter_Level.
 * 		Moptop_General_Log_Filter_Level_Absolute and 
 *              Moptop_General_Log_Filter_Level_Bitwise test it against
 * 		message levels to determine whether to log messages.</dd>
 * <dt>Log_Directory</dt> <dd>Directory to write log messages to.</dd>
 * <dt>Log_Filename_Root</dt> <dd>Root of filename to write log messages to.</dd>
 * <dt>Log_Filename</dt> <dd>Filename to write log messages to.</dd>
 * <dt>Log_FP</dt> <dd>File pointer to write log messages to.</dd>
 * <dt>Error_Filename_Root</dt> <dd>Root of filename to write error messages to.</dd>
 * <dt>Error_Filename</dt> <dd>Filename to write error messages to.</dd>
 * <dt>Error_FP</dt> <dd>File pointer to write error messages to.</dd>
 * <dt>Config_Filename</dt> <dd>String containing the filename of the config file. Must be allocated before use.</dd>
 * <dt>Log_UDP_Active</dt> <dd>A boolean, TRUE if we should send log records to the log server using UDP.</dd>
 * <dt>Log_UDP_Hostname</dt> <dd>String containing the Hostname to send log_udp records to.</dd>
 * <dt>Log_UDP_Port_Number</dt> <dd>The port number to send log_udp records to.</dd>
 * <dt>Log_UDP_Socket_Id</dt> <dd>The socket_id of the opened socket to the log server..</dd>
 * </dl>
 * @see #LOG_HANDLER_LIST_COUNT
 * @see #Log_Handler_Function
 * @see #Moptop_General_Log
 * @see #Moptop_General_Set_Log_Filter_Level
 * @see #Moptop_General_Log_Filter_Level_Absolute
 * @see #Moptop_General_Log_Filter_Level_Bitwise
 */
struct General_Struct
{
	Log_Handler_Function Log_Handler_List[LOG_HANDLER_LIST_COUNT];
	int (*Log_Filter)(char *sub_system,char *source_filename,char *function,int level,char *category,
			  char *message);
	int Log_Filter_Level;
	char Log_Directory[MOPTOP_GENERAL_FILENAME_LENGTH];
	char Log_Filename_Root[MOPTOP_GENERAL_FILENAME_LENGTH];
	char Log_Filename[MOPTOP_GENERAL_FILENAME_LENGTH];
	FILE *Log_Fp;
	char Error_Filename_Root[MOPTOP_GENERAL_FILENAME_LENGTH];
	char Error_Filename[MOPTOP_GENERAL_FILENAME_LENGTH];
	FILE *Error_Fp;
	char *Config_Filename;
	int Log_UDP_Active;
	char Log_UDP_Hostname[MOPTOP_GENERAL_FILENAME_LENGTH];
	int Log_UDP_Port_Number;
	int Log_UDP_Socket_Id;
};

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of General_Struct that contains local data for this module.
 * This is statically initialised to the following:
 * <dl>
 * <dt>Log_Handler_List</dt> <dd>{NULL,NULL,NULL,NULL,NULL}</dd>
 * <dt>Log_Filter</dt> <dd>NULL</dd>
 * <dt>Log_Filter_Level</dt> <dd>0</dd>
 * <dt>Log_Directory</dt> <dd>NULL</dd>
 * <dt>Log_Filename_Root</dt> <dd>moptop_c_log</dd>
 * <dt>Log_Filename</dt> <dd>moptop_c_log.txt</dd>
 * <dt>Log_FP</dt> <dd>NULL</dd>
 * <dt>Error_Filename_Root</dt> <dd>moptop_c_error</dd>
 * <dt>Error_Filename</dt> <dd>moptop_c_error.txt</dd>
 * <dt>Error_FP</dt> <dd>NULL</dd>
 * <dt>Config_Filename</dt> <dd>NULL</dd>
 * <dt>Log_UDP_Active</dt> <dd>FALSE</dd>
 * <dt>Log_UDP_Hostname</dt> <dd>""</dd>
 * <dt>Log_UDP_Port_Number</dt> <dd>0</dd>
 * <dt>Log_UDP_Socket_Id</dt> <dd>0</dd>
 * </dl>
 * @see #General_Struct
 */
static struct General_Struct General_Data = 
{
        {NULL,NULL,NULL,NULL,NULL},NULL,0,"","moptop_c_log","moptop_c_log.txt",NULL,
	"moptop_c_error","moptop_c_error.txt",NULL,NULL,FALSE,"",0,-1
};

/* internal functions */
static void General_Log_Handler_Hourly_File_Set_Fp(char *directory,char *basename,char *log_filename,FILE **log_fp);
static void General_Log_Handler_Get_Hourly_Filename(char *directory,char *basename,char *filename);
static void General_Log_Handler_Filename_To_Fp(char *log_filename,FILE **log_fp);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * A general error routine. 
 * <b>Note</b> you cannot call both Moptop_General_Error and Moptop_General_Error_To_String 
 * to print the error string and 
 * get a string copy of it, only one of the error routines can be called after an error has been generated .
 * A second call to one of these routines will generate a 'Error not found' error!.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @see #Moptop_General_Get_Current_Time_String
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 * @see #General_Data
 * @see ../commandserver/cdocs/command_server.html#Command_Server_Is_Error
 * @see ../commandserver/cdocs/command_server.html#Command_Server_Error_To_String
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Is_Error
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Error_To_String
 */
void Moptop_General_Error(char *sub_system,char *source_filename,char *function,int level,char *category)
{
	char buff[MOPTOP_GENERAL_ERROR_STRING_LENGTH];
	char time_string[32];
	int found = FALSE;

	/* change error files if necessary */
	General_Log_Handler_Hourly_File_Set_Fp(General_Data.Log_Directory,General_Data.Error_Filename_Root,
					       General_Data.Error_Filename,&General_Data.Error_Fp);
	if(General_Data.Error_Fp == NULL)
	{
		fprintf(stderr,"Failed to set error Fp.\n");
		fflush(stderr);
		General_Data.Error_Fp = stderr;
	}
	strcpy(buff,"");
	strcpy(buff,"");
	if(Command_Server_Is_Error())
	{
		found = TRUE;
		Command_Server_Error_To_String(buff);
		fprintf(General_Data.Error_Fp,"\t%s\n",buff);
		fflush(General_Data.Error_Fp);
	}
	strcpy(buff,"");
	strcpy(buff,"");
	if(CCD_General_Is_Error())
	{
		found = TRUE;
		CCD_General_Error_To_String(buff);
		fprintf(General_Data.Error_Fp,"\t%s\n",buff);
		fflush(General_Data.Error_Fp);
	}
	if(Moptop_General_Error_Number != 0)
	{
		found = TRUE;
		Moptop_General_Get_Current_Time_String(time_string,32);
		fprintf(General_Data.Error_Fp,"%s Moptop_General:Error(%d) : %s:%s\n",time_string,
			Moptop_General_Error_Number,function,Moptop_General_Error_String);
		fflush(General_Data.Error_Fp);
	}
	if(!found)
	{
		fprintf(General_Data.Error_Fp,"Error:Moptop_General_Error:Error not found\n");
		fflush(General_Data.Error_Fp);
	}
}

/**
 * A general error routine.
 * <b>Note</b> you cannot call both Moptop_General_Error and Moptop_General_Error_To_String 
 * to print the error string and 
 * get a string copy of it, only one of the error routines can be called after libccd has generated an error.
 * A second call to one of these routines will generate a 'Error not found' error!.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param error_string A character buffer big enough to store the longest possible error message. It is
 * recomended that it is at least 1024 bytes in size.
 * @see #Moptop_General_Get_Current_Time_String
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 */
void Moptop_General_Error_To_String(char *sub_system,char *source_filename,char *function,int level,
					char *category,char *error_string)
{
	char time_string[32];

	strcpy(error_string,"");
	if(Moptop_General_Error_Number != 0)
	{
		Moptop_General_Get_Current_Time_String(time_string,32);
		sprintf(error_string+strlen(error_string),"%s Moptop_General:Error(%d) : %s:%s\n",time_string,
			Moptop_General_Error_Number,function,Moptop_General_Error_String);
	}
	if(strlen(error_string) == 0)
	{
		strcat(error_string,"Error:Moptop_General_Error:Error not found\n");
	}
}

/**
 * Routine to get the current time in a string. The string is returned in the format
 * '01/01/2000 13:59:59', or the string "Unknown time" if the routine failed.
 * The time is in UTC.
 * We now use gmtime_r, as there is a possibility of some status commands calling gmtime whilst
 * FITS headers are being formatted with gmtime, which can cause corrupted FITS headers (Fault #2096).
 * @param time_string The string to fill with the current time.
 * @param string_length The length of the buffer passed in. It is recommended the length is at least 20 characters.
 * @see #MOPTOP_GENERAL_ONE_MILLISECOND_NS
 */
void Moptop_General_Get_Current_Time_String(char *time_string,int string_length)
{
	char timezone_string[16];
	char millsecond_string[8];
	struct timespec current_time;
	struct tm utc_time;

	clock_gettime(CLOCK_REALTIME,&current_time);
	gmtime_r(&(current_time.tv_sec),&utc_time);
	strftime(time_string,string_length,"%d-%m-%YT%H:%M:%S",&utc_time);
	sprintf(millsecond_string,"%03d",(current_time.tv_nsec/MOPTOP_GENERAL_ONE_MILLISECOND_NS));
	strftime(timezone_string,16,"%z",&utc_time);
	if((strlen(time_string)+strlen(millsecond_string)+strlen(timezone_string)+3) < string_length)
	{
		strcat(time_string,".");
		strcat(time_string,millsecond_string);
		strcat(time_string," ");
		strcat(time_string,timezone_string);
	}
}

/**
 * Routine to log a message to a defined logging mechanism. This routine has an arbitary number of arguments,
 * and uses vsprintf to format them i.e. like fprintf. The General_Buff is used to hold the created string,
 * therefore the total length of the generated string should not be longer than MOPTOP_GENERAL_ERROR_STRING_LENGTH.
 * Moptop_General_Log is then called to handle the log message.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param format A string, with formatting statements the same as fprintf would use to determine the type
 * 	of the following arguments.
 * @see #Moptop_General_Log
 * @see #MOPTOP_GENERAL_ERROR_STRING_LENGTH
 */
void Moptop_General_Log_Format(char *sub_system,char *source_filename,char *function,int level,
				   char *category,char *format,...)
{
	va_list ap;
	char buff[MOPTOP_GENERAL_ERROR_STRING_LENGTH];

/* format the arguments */
	va_start(ap,format);
	vsprintf(buff,format,ap);
	va_end(ap);
/* call the log routine to log the results */
	Moptop_General_Log(sub_system,source_filename,function,level,category,buff);
}

/**
 * Routine to log a message to a defined logging mechanism. If the string or General_Data.Log_Handler are NULL
 * the routine does not log the message. If the General_Data.Log_Filter function pointer is non-NULL, the
 * message is passed to it to determoine whether to log the message.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param message The message to log.
 * @see #General_Data
 * @see #LOG_HANDLER_LIST_COUNT
 */
void Moptop_General_Log(char *sub_system,char *source_filename,char *function,int level,
			    char *category,char *message)
{
	int i;

/* If the string is NULL, don't log. */
	if(message == NULL)
		return;
/* If there's a log filter, check it returns TRUE for this message */
	if(General_Data.Log_Filter != NULL)
	{
		if(General_Data.Log_Filter(sub_system,source_filename,function,level,category,message) == FALSE)
			return;
	}
/* We can log the message */
	Moptop_General_Call_Log_Handlers(sub_system,source_filename,function,level,category,message);
}

/**
 * Routine that goes through the General_Data.Log_Handler_List and invokes each non-null handler.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param message The message to log.
 */
void Moptop_General_Call_Log_Handlers(char *sub_system,char *source_filename,char *function,int level,
					  char *category,char *message)
{
	int i;

	for(i=0;i<LOG_HANDLER_LIST_COUNT;i++)
	{
		if(General_Data.Log_Handler_List[i] != NULL)
		{
			(*(General_Data.Log_Handler_List[i]))(sub_system,source_filename,function,level,category,
							      message);
		}
	}
}

/**
 * Routine that goes through the General_Data.Log_Handler_List and invokes each non-null handler for logging
 * of the CCD subsystem. We call Moptop_General_Call_Log_Handlers with parameters as follows: 
 * "sub_system" and "category" are set to "CCD". 
 * "source_filename" and "function" are set to NULL. "level" and "message" are passed through.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param message The message to log.
 * @see #Moptop_General_Call_Log_Handlers
 */
void Moptop_General_Call_Log_Handlers_CCD(int level,char *message)
{
	Moptop_General_Call_Log_Handlers("CCD",NULL,NULL,level,"CCD",message);
}

/**
 * Routine to add the log handler to the General_Data.Log_Handler_List used by Moptop_General_Log.
 * @param log_fn A function pointer to a suitable handler.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #LOG_HANDLER_LIST_COUNT
 * @see #General_Data
 * @see #Moptop_General_Log
 */
int Moptop_General_Add_Log_Handler_Function(void (*log_fn)(char *sub_system,char *source_filename,
						 char *function,int level,char *category,char *message))
{
	int index;

	index = 0;
	/* find empty index */
	while((index < LOG_HANDLER_LIST_COUNT)&&(General_Data.Log_Handler_List[index] != NULL))
		index++;
	if(index == LOG_HANDLER_LIST_COUNT)
	{
		Moptop_General_Error_Number = 113;
		sprintf(Moptop_General_Error_String,"Moptop_General_Add_Log_Handler_Function:"
			"Could not find empty entry in list for %p (%d).",log_fn,index);
		return FALSE;
	}
	/* set empty index to be log fn */
	General_Data.Log_Handler_List[index] = log_fn;
	return TRUE;
}

/**
 * Routine to set the General_Data.Log_Filter used by Moptop_General_Log.
 * @param log_fn A function pointer to a suitable filter function.
 * @see #General_Data
 * @see #Moptop_General_Log
 */
void Moptop_General_Set_Log_Filter_Function(int (*filter_fn)(char *sub_system,char *source_filename,
						char *function,int level,char *category,char *string))
{
	General_Data.Log_Filter = filter_fn;
}

/**
 * Sets the contents of the General_Data.Log_Directory, used for constructing log/error filenames.
 * @param directory The directory name.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #MOPTOP_GENERAL_FILENAME_LENGTH
 */
int Moptop_General_Log_Set_Directory(char *directory)
{
	if(directory == NULL)
	{
		Moptop_General_Error_Number = 103;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_Directory:directory was NULL.");
		return FALSE;
	}
	if((MOPTOP_GENERAL_FILENAME_LENGTH - strlen(directory)) < 10)
	{
		Moptop_General_Error_Number = 104;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_Directory:"
			"directory was too long (%d vs %d).",strlen(directory),MOPTOP_GENERAL_FILENAME_LENGTH);
		return FALSE;
	}
	strcpy(General_Data.Log_Directory,directory);
	return TRUE;
}

/**
 * Sets the contents of the General_Data.Log_Filename_Root, used for constructing log filenames.
 * @param filename_root The root of filenames.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #MOPTOP_GENERAL_FILENAME_LENGTH
 */
int Moptop_General_Log_Set_Root(char *filename_root)
{
	if(filename_root == NULL)
	{
		Moptop_General_Error_Number = 116;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_Root:filename_root was NULL.");
		return FALSE;
	}
	if((MOPTOP_GENERAL_FILENAME_LENGTH - strlen(filename_root)) < 10)
	{
		Moptop_General_Error_Number = 117;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_Root:"
			"filename root was too long (%d vs %d).",strlen(filename_root),MOPTOP_GENERAL_FILENAME_LENGTH);
		return FALSE;
	}
	strcpy(General_Data.Log_Filename_Root,filename_root);
	return TRUE;
}

/**
 * Sets the contents of the General_Data.Error_Filename_Root, used for constructing error filenames.
 * @param filename_root The root of filenames.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #MOPTOP_GENERAL_FILENAME_LENGTH
 */
int Moptop_General_Log_Set_Error_Root(char *filename_root)
{
	if(filename_root == NULL)
	{
		Moptop_General_Error_Number = 118;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_Error_Root:filename_root was NULL.");
		return FALSE;
	}
	if((MOPTOP_GENERAL_FILENAME_LENGTH - strlen(filename_root)) < 10)
	{
		Moptop_General_Error_Number = 119;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_Error_Root:"
			"filename root was too long (%d vs %d).",strlen(filename_root),MOPTOP_GENERAL_FILENAME_LENGTH);
		return FALSE;
	}
	strcpy(General_Data.Error_Filename_Root,filename_root);
	return TRUE;
}

/**
 * Sets the contents of the General_Data fields used for log_udp calls.
 * @param active Whether to send log_udp packets, a boolean.
 * @param hostname The hostname to send the UDP packet to.
 * @param port_number The port number to send the UDP packet to.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #General_Data
 * @see #MOPTOP_GENERAL_FILENAME_LENGTH
 */
int Moptop_General_Log_Set_UDP(int active,char *hostname,int port_number)
{
	General_Data.Log_UDP_Active = active;
	if(hostname == NULL)
	{
		Moptop_General_Error_Number = 111;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_UDP:hostname was NULL.");
		return FALSE;
	}
	if(strlen(hostname) >= (MOPTOP_GENERAL_FILENAME_LENGTH-1))
	{
		Moptop_General_Error_Number = 112;
		sprintf(Moptop_General_Error_String,"Moptop_General_Log_Set_UDP:"
			"hostname was too long (%d vs %d).",strlen(hostname),MOPTOP_GENERAL_FILENAME_LENGTH);
		return FALSE;
	}
	strcpy(General_Data.Log_UDP_Hostname,hostname);
	General_Data.Log_UDP_Port_Number = port_number;
	return TRUE;
}

/**
 * A log handler to be used for the General_Data.Log_Handler function.
 * Just prints the message to stdout, terminated by a newline.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param message The log message to be logged. 
 */
void Moptop_General_Log_Handler_Stdout(char *sub_system,char *source_filename,char *function,int level,
						  char *category,char *message)
{
	if(message == NULL)
		return;
	fprintf(stdout,"%s:%s\n",function,message);
}

/**
 * A log handler to be used for the General_Data.Log_Handler function.
 * Prints the message to General_Data.Log_Fp, terminated by a newline, and thenflushes the stream.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param message The log message to be logged. 
 * @see #General_Data
 */
void Moptop_General_Log_Handler_Log_Fp(char *sub_system,char *source_filename,char *function,int level,
						  char *category,char *message)
{
	if(message == NULL)
		return;
	fprintf(General_Data.Log_Fp,"%s:%s\n",function,message);
	fflush(General_Data.Log_Fp);
}

/**
 * A log handler to be used for the General_Data.Log_Handler function.
 * First calls General_Log_Handler_Hourly_File_Set_Fp to open/check the right log file is open.
 * Prints the message to General_Data.Log_Fp, terminated by a newline, and then flushes the stream.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param string The log message to be logged. 
 * @see #Moptop_General_Get_Current_Time_String
 * @see #General_Data
 * @see #General_Log_Handler_Log_Hourly_File_Set_Fp
 */
void Moptop_General_Log_Handler_Log_Hourly_File(char *sub_system,char *source_filename,char *function,
						    int level,char *category,char *message)
{
	char time_string[32];

	if(message == NULL)
		return;
	General_Log_Handler_Hourly_File_Set_Fp(General_Data.Log_Directory,General_Data.Log_Filename_Root,
					       General_Data.Log_Filename,&General_Data.Log_Fp);
	Moptop_General_Get_Current_Time_String(time_string,32);
	fprintf(General_Data.Log_Fp,"%s : %s:%s\n",time_string,function,message);
	fflush(General_Data.Log_Fp);
}

/**
 * A log handler to be used as a handler in the the General_Data.Log_Handler_List function.
 * If General_Data.Log_UDP_Active is TRUE:
 * <ul>
 * <li>If the General_Data.Log_UDP_Socket_Id is less than 0, call Log_UDP_Open to open a socket.
 * <li>Call Log_Create_Record to create a log record.
 * <li>Call Log_UDP_Send to send the log record as a UDP packet.
 * </ul>
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param string The log message to be logged. 
 * @see #General_Data
 * @see ../../log_udp/cdocs/log_udp.html#Log_UDP_Open
 * @see ../../log_udp/cdocs/log_udp.html#Log_UDP_Send
 * @see ../../log_udp/cdocs/log_create.html#Log_Create_Record
 * @see ../../log_udp/cdocs/log_general.htmlLog_General_Error
 */
void Moptop_General_Log_Handler_Log_UDP(char *sub_system,char *source_filename,char *function,
						    int level,char *category,char *message)
{
	struct Log_Record_Struct log_record;

	if(General_Data.Log_UDP_Active)
	{
		/* if the socket is not already open, open it */
		if(General_Data.Log_UDP_Socket_Id < 0)
		{
			if(!Log_UDP_Open(General_Data.Log_UDP_Hostname,General_Data.Log_UDP_Port_Number,
					 &(General_Data.Log_UDP_Socket_Id)))
			{
				Log_General_Error();
				General_Data.Log_UDP_Socket_Id = -1;
			}
		}
		/* create a log record */
		if(!Log_Create_Record("MOPTOP",sub_system,source_filename,NULL,function,LOG_SEVERITY_INFO,
				      level,category,message,&log_record))
		{
			Log_General_Error();
		}
		if(General_Data.Log_UDP_Socket_Id > 0)
		{
			if(!Log_UDP_Send(General_Data.Log_UDP_Socket_Id,log_record,0,NULL))
			{
				Log_General_Error();
				General_Data.Log_UDP_Socket_Id = -1;
			}
		}
	}
}

/**
 * Routine to set the General_Data.Log_Filter_Level.
 * @see #General_Data
 */
void Moptop_General_Set_Log_Filter_Level(int level)
{
	General_Data.Log_Filter_Level = level;
}

/**
 * A log message filter routine, to be used for the General_Data.Log_Filter function pointer.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level is less than or equal to the General_Data.Log_Filter_Level,
 * 	otherwise it returns FALSE.
 * @see #General_Data
 */
int Moptop_General_Log_Filter_Level_Absolute(char *sub_system,char *source_filename,char *function,
						 int level,char *category,char *message)
{
	return (level <= General_Data.Log_Filter_Level);
}

/**
 * A log message filter routine, to be used for the General_Data.Log_Filter function pointer.
 * @param sub_system The sub system. Can be NULL.
 * @param source_file The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level has bits set that are also set in the 
 * 	General_Data.Log_Filter_Level, otherwise it returns FALSE.
 * @see #General_Data
 */
int Moptop_General_Log_Filter_Level_Bitwise(char *sub_system,char *source_filename,char *function,
						       int level,char *category,char *message)
{
	return ((level & General_Data.Log_Filter_Level) > 0);
}

/**
 * Utility function to add a string to an allocated string.
 * @param string The address of a pointer to a string. If a new string, the pointer should have
 *             been initialised to NULL.
 * @param add A non-null string to apend to the current contents of string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 */
int Moptop_General_Add_String(char **string,char *add)
{
	if(string == NULL)
	{
		Moptop_General_Error_Number = 100;
		sprintf(Moptop_General_Error_String,"Moptop_General_Add_String:"
			"NULL string pointer to add %s to.",add);
		return FALSE;
	}
	if(add == NULL)
		return TRUE;
	if((*string) == NULL)
	{
		(*string) = (char*)malloc((strlen(add)+1)*sizeof(char));
		if((*string) != NULL)
			(*string)[0] = '\0';
	}
	else
		(*string) = (char*)realloc((*string),(strlen((*string))+strlen(add)+1)*sizeof(char));
	if((*string) == NULL)
	{
		Moptop_General_Error_Number = 101;
		sprintf(Moptop_General_Error_String,"Moptop_General_Add_String:Memory allocation error (%s).",add);
		return FALSE;
	}
	strcat((*string),add);
	return TRUE;
}

/**
 * Add an integer to a list of integers.
 * @param add The integer value to add.
 * @param list The address of a list of integers.
 * @param count The address of an integer storing the number of integers in the list.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 */
int Moptop_General_Int_List_Add(int add,int **list,int *count)
{
	if(list == NULL)
	{
		Moptop_General_Error_Number = 108;
		sprintf(Moptop_General_Error_String,"Moptop_General_Int_List_Add:list was NULL.");
		return FALSE;
	}
	if(count == NULL)
	{
		Moptop_General_Error_Number = 109;
		sprintf(Moptop_General_Error_String,"Moptop_General_Int_List_Add:count was NULL.");
		return FALSE;
	}
	if((*list)==NULL)
		(*list) = (int*)malloc(sizeof(int));
	else
		(*list) = (int*)realloc(*list,((*count) + 1)*sizeof(int));
	if((*list)==NULL)
	{
		Moptop_General_Error_Number = 110;
		sprintf(Moptop_General_Error_String,"Moptop_General_Int_List_Add:list realloc failed(%d).",
			(*count));
		return FALSE;
	}
	(*list)[(*count)++] = add;
	return TRUE;
}

/**
 * Routine to pass to qsort to sort a list on integers in ascending order.
 */
int Moptop_General_Int_List_Sort(const void *f,const void *s)
{
	return (*(int*)f) - (*(int*)s);
}

/**
 * Routine to lock a access mutex. This will block until the mutex has been acquired,
 * unless an error occurs.
 * @param mutex A pointer to the variable containing the mutex to be locked.
 * @return Returns TRUE if the mutex has been  locked for access by this thread,
 * 	FALSE if an error occured.
 */
int Moptop_General_Mutex_Lock(pthread_mutex_t *mutex)
{
	int error_number;

	error_number = pthread_mutex_lock(mutex);
	if(error_number != 0)
	{
		Moptop_General_Error_Number = 102;
		sprintf(Moptop_General_Error_String,"Moptop_General_Mutex_Lock:Mutex lock failed '%d'.",
			error_number);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to unlock the mutex. 
 * @param mutex A pointer to the variable containing the mutex to be unlocked.
 * @return Returns TRUE if the mutex has been unlocked, FALSE if an error occured.
 * @see #DSP_Data
 */
int Moptop_General_Mutex_Unlock(pthread_mutex_t *mutex)
{
	int error_number;

	error_number = pthread_mutex_unlock(mutex);
	if(error_number != 0)
	{
		Moptop_General_Error_Number = 105;
		sprintf(Moptop_General_Error_String,"Moptop_General_Mutex_Unlock:Mutex unlock failed '%d'.",
			error_number);
		return FALSE;
	}
	return TRUE;
}

/**
 * Set the priority of the thread running this routine to the "normal" priority.
 * @return The routine returns TRUE if the priority of the thread that has called this function is changed,
 *         and FALSE otherwise.
 * @see moptop_config.html#Moptop_Config_Get_Integer
 * @see moptop_general.html#Moptop_General_Thread_Priority_Set
 */
int Moptop_General_Thread_Priority_Set_Normal(void)
{
	int normal_priority;

	if(!Moptop_Config_Get_Integer("thread.priority.normal",&normal_priority))
		return FALSE;
	if(!Moptop_General_Thread_Priority_Set(normal_priority))
		return FALSE;
	return TRUE;
}

/**
 * Set the priority of the thread running this routine to the "exposure" priority.
 * @return The routine returns TRUE if the priority of the thread that has called this function is changed,
 *         and FALSE otherwise.
 * @see moptop_config.html#Moptop_Config_Get_Integer
 * @see moptop_general.html#Moptop_General_Thread_Priority_Set
 */
int Moptop_General_Thread_Priority_Set_Exposure(void)
{
	int exposure_priority;

	if(!Moptop_Config_Get_Integer("thread.priority.exposure",&exposure_priority))
		return FALSE;
	if(!Moptop_General_Thread_Priority_Set(exposure_priority))
		return FALSE;
	return TRUE;
}

/**
 * Routine to set the priority of the thread that has called this function. The scheduler policy SCHED_FIFO is used.
 * @param priority The priority level to use. This is normally 1..99 for SCHED_FIFO under linux, the higher the number
 * the more likely the thread is to run first. Under SCHED_FIFO the highest priority thread that can run is scheduled 
 * next, and can pre-empt a thread with a lower priority.
 * @return The routine returns TRUE if the priority of the thread that has called this function is changed,
 *         and FALSE otherwise.
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 */
int Moptop_General_Thread_Priority_Set(int priority)
{
	struct sched_param p;
	int min_priority,max_priority,retval;

	min_priority = sched_get_priority_min(SCHED_FIFO);
	max_priority = sched_get_priority_max(SCHED_FIFO);
	if((priority < min_priority) || (priority > max_priority))
	{
		Moptop_General_Error_Number = 114;
		sprintf(Moptop_General_Error_String,
			"Moptop_General_Thread_Priority_Set:Priority'%d' out of range (%d,%d).",
			priority,min_priority,max_priority);
		return FALSE;
	}
	p.sched_priority = priority;
	retval = pthread_setschedparam(pthread_self(),SCHED_FIFO,&p);
	if(retval != 0)
	{
		Moptop_General_Error_Number = 115;
		sprintf(Moptop_General_Error_String,
			"Moptop_General_Thread_Priority_Set:Setting thread priority to '%d' failed (%d).",
			priority,retval);
		return FALSE;
	}
	return TRUE;
}

/**
 * Set the config filename.
 * @param filename The string to copy.
 * @return Returns TRUE on success and FALSE on failure.
 * @see #General_Data
 */
int Moptop_General_Set_Config_Filename(char *filename)
{
	if(filename == NULL)
	{
		Moptop_General_Error_Number = 106;
		sprintf(Moptop_General_Error_String,"Moptop_General_Set_Config_Filename:Filename was NULL.");
		return FALSE;
	}
	General_Data.Config_Filename = strdup(filename);
	if(General_Data.Config_Filename == NULL)
	{
		Moptop_General_Error_Number = 107;
		sprintf(Moptop_General_Error_String,"Moptop_General_Set_Config_Filename:strdup(%s) failed.",
			filename);
		return FALSE;
	}
	return TRUE;
}

/**
 * Get the config filename.
 * @return Returns the current config filename. This can be NULL if it has not been set properly.
 * @see #General_Data
 */
char *Moptop_General_Get_Config_Filename(void)
{
	return General_Data.Config_Filename;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Set up a log/error file FP correctly.
 * @param directory The directory.
 * @param basename The basename, either "moptop_c_log"/"moptop_c_error".
 * @param log_filename A string pointer to fill with the current/new log/error filename.
 * @param log_fp The address of a FILE pointer to fill with the new fp, if the filename changed, otherwise
 *               remains the same.
 */
static void General_Log_Handler_Hourly_File_Set_Fp(char *directory,char *basename,char *log_filename,FILE **log_fp)
{
	char new_filename[MOPTOP_GENERAL_FILENAME_LENGTH];

	if((*log_fp) == NULL)
	{
		General_Log_Handler_Get_Hourly_Filename(directory,basename,log_filename);
		General_Log_Handler_Filename_To_Fp(log_filename,log_fp);
	}
	else
	{
		General_Log_Handler_Get_Hourly_Filename(directory,basename,new_filename);
		if(strcmp(new_filename,log_filename) != 0)
		{
			fflush(*log_fp);
			fclose(*log_fp);
			(*log_fp) = NULL;
			strcpy(log_filename,new_filename);
			General_Log_Handler_Filename_To_Fp(log_filename,log_fp);
		}
	}
}

/**
 * Given a directory and  basename, set a filename.
 * @param directory The directory string (can be NULL).
 * @param basename The log file basename (e.g. log/error), cannot be NULL.
 * @param filename A string to fill in with the constructed filename.
 */
static void General_Log_Handler_Get_Hourly_Filename(char *directory,char *basename,char *filename)
{
	time_t now_time;
	int doy,hod;
	struct tm *now_tm;

	if(filename == NULL)
		return;
	now_time = time(NULL);
	if(now_time == (time_t)-1)
	{
		/* what do I do here? */
		strcpy(filename,basename);
		return;
	}
	now_tm = gmtime(&now_time);
	if(now_tm == NULL)
	{
		/* what do I do here? */
		strcpy(filename,basename);
		return;
	}
	doy = now_tm->tm_yday+1; /* tm_yday starts at zero, RJS wants Jan 1st to be day 1 */
	hod = now_tm->tm_hour;
	if(directory != NULL)
	{
		sprintf(filename,"%s/%s_%03d_%02d.txt",directory,basename,doy,hod);
	}
	else
	{
		sprintf(filename,"%s_%03d_%02d.txt",basename,doy,hod);
	}
}

/**
 * Try to open the specified file for appending.
 * The FP will not be opened if it is NULL, the filename is NULL, or the fopen fails.
 * @param log_filename The string containing the filename to open.
 * @param log_fp The address of a pointer to open the Fp.
 */
static void General_Log_Handler_Filename_To_Fp(char *log_filename,FILE **log_fp)
{
	int fopen_errno;

	if(log_fp == NULL)
	{
		return;
	}
	if(log_filename == NULL)
	{
		(*log_fp) = NULL;
		return;
	}
	(*log_fp) = fopen(log_filename,"a");
	/* (*log_fp) can still be NULL here on failure */
	if((*log_fp) == NULL)
	{
		fopen_errno = errno;
		fprintf(stderr,"General_Log_Handler_Filename_To_Fp:fopen '%s' failed %d.\n",log_filename,
			fopen_errno);
	}
}

