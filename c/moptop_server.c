/* moptop_server.c
** Moptop server routines
*/
/**
 * Command Server routines for the moptop program.
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

#include "command_server.h"

#include "moptop_config.h"
#include "moptop_general.h"
#include "moptop_command.h"
#include "moptop_server.h"

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The server context to use for this server.
 * @see ../command_server/cdocs/command_server.html#Command_Server_Server_Context_T
 */
static Command_Server_Server_Context_T Command_Server_Context = NULL;
/**
 * Command server port number.
 */
static unsigned short Command_Server_Port_Number = 1234;

/* internal functions */
static void Server_Connection_Callback(Command_Server_Handle_T connection_handle);
static int Send_Reply(Command_Server_Handle_T connection_handle,char *reply_message);
static int Send_Binary_Reply(Command_Server_Handle_T connection_handle,void *buffer_ptr,size_t buffer_length);
static int Send_Binary_Reply_Error(Command_Server_Handle_T connection_handle);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Moptop server initialisation routine. Assumes Moptop_Config_Load has previously been called
 * to load the configuration file.
 * It loads the unsigned short with key ""command.server.port_number" into the Command_Server_Port_Number variable
 * for use in Moptop_Server_Start.
 * @return The routine returns TRUE if successfull, and FALSE if an error occurs.
 * @see #Moptop_Server_Start
 * @see #Command_Server_Port_Number
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Get_Unsigned_Short
 */
int Moptop_Server_Initialise(void)
{
	int retval;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("server","moptop_server.c","Moptop_Server_Initialise",
				  LOG_VERBOSITY_TERSE,"SERVER","started.");
#endif
	/* get port number from config */
	retval = Moptop_Config_Get_Unsigned_Short("command.server.port_number",&Command_Server_Port_Number);
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 200;
		sprintf(Moptop_General_Error_String,"Failed to find port number in config file.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("server","moptop_server.c","Moptop_Server_Initialise",
				  LOG_VERBOSITY_TERSE,"SERVER","finished.");
#endif
	return TRUE;
}

/**
 * Moptop server start routine.
 * This routine starts the server. It does not return until the server is stopped using Moptop_Server_Stop.
 * @return The routine returns TRUE if successfull, and FALSE if an error occurs.
 * @see #Command_Server_Port_Number
 * @see #Server_Connection_Callback
 * @see #Command_Server_Context
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../command_server/cdocs/command_server.html#Command_Server_Start_Server
 */
int Moptop_Server_Start(void)
{
	int retval;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("server","moptop_server.c","Moptop_Server_Start",
			   LOG_VERBOSITY_VERY_TERSE,"SERVER","started.");
#endif
#if MOPTOP_DEBUG > 2
	Moptop_General_Log_Format("server","moptop_server.c","Moptop_Server_Start",
				      LOG_VERBOSITY_VERY_TERSE,"SERVER",
				      "Starting multi-threaded server on port %hu.",Command_Server_Port_Number);
#endif
	retval = Command_Server_Start_Server(&Command_Server_Port_Number,Server_Connection_Callback,
					     &Command_Server_Context);
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 201;
		sprintf(Moptop_General_Error_String,"Moptop_Server_Start:"
			"Command_Server_Start_Server returned FALSE.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("server","moptop_server.c","Moptop_Server_Start",
			   LOG_VERBOSITY_VERY_TERSE,"SERVER","finished.");
#endif
	return TRUE;
}

/**
 * Moptop server stop routine.
 * @return The routine returns TRUE if successfull, and FALSE if an error occurs.
 * @see #Command_Server_Context
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see ../command_server/cdocs/command_server.html#Command_Server_Close_Server
 */
int Moptop_Server_Stop(void)
{
	int retval;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("server","moptop_server.c","Moptop_Server_Stop",
				      LOG_VERBOSITY_VERY_TERSE,"SERVER","started.");
#endif
	retval = Command_Server_Close_Server(&Command_Server_Context);
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 202;
		sprintf(Moptop_General_Error_String,"Moptop_Server_Stop:"
			"Command_Server_Close_Server returned FALSE.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("server","moptop_server.c","Moptop_Server_Stop",
				      LOG_VERBOSITY_VERY_TERSE,"SERVER","finished.");
#endif
	return TRUE;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Server connection thread, invoked whenever a new command comes in.
 * @param connection_handle Connection handle for this thread.
 * @see #Send_Reply
 * @see #Send_Binary_Reply
 * @see #Send_Binary_Reply_Error
 * @see #Moptop_Server_Stop
 * @see moptop_command.html#Moptop_Command_Abort
 * @see moptop_command.html#Moptop_Command_Config
 * @see moptop_command.html#Moptop_Command_Fits_Header
 * @see moptop_command.html#Moptop_Command_Multrun
 * @see moptop_command.html#Moptop_Command_Mult_Dark
 * @see moptop_command.html#Moptop_Command_Status
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Thread_Priority_Set_Normal
 * @see moptop_general.html#Moptop_General_Thread_Priority_Set_Exposure
 * @see ../command_server/cdocs/command_server.html#Command_Server_Read_Message
 */
static void Server_Connection_Callback(Command_Server_Handle_T connection_handle)
{
	void *buffer_ptr = NULL;
	size_t buffer_length = 0;
	char *reply_string = NULL;
	char *client_message = NULL;
	int retval;
	int seconds,i;

	/* get message from client */
	retval = Command_Server_Read_Message(connection_handle, &client_message);
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 203;
		sprintf(Moptop_General_Error_String,"Moptop_Server_Connection_Callback:"
			"Failed to read message.");
		Moptop_General_Error("server","moptop_server.c","Moptop_Server_Connection_Callback",
					 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		return;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("server","moptop_server.c","Moptop_Server_Connection_Callback",
				      LOG_VERBOSITY_VERY_TERSE,"SERVER","received '%s'",client_message);
#endif
	/* do something with message */
	if(strncmp(client_message,"abort",5) == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","abort detected.");
#endif
		/* exposure thread priority */
		if(!Moptop_General_Thread_Priority_Set_Exposure())
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Moptop_Command_Abort(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Moptop_Command_Abort failed.");
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
						     "Moptop_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strncmp(client_message,"config",6) == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","config detected.");
#endif
		/* normal thread priority */
		if(!Moptop_General_Thread_Priority_Set_Normal())
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Moptop_Command_Config(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Moptop_General_Error("server","moptop_server.c",
					     "Moptop_Server_Connection_Callback",
					     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Moptop_Command_Config failed.");
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
						     "Moptop_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strncmp(client_message,"fitsheader",10) == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","fitsheader detected.");
#endif
		/* normal thread priority */
		if(!Moptop_General_Thread_Priority_Set_Normal())
		{
			Moptop_General_Error("server","moptop_server.c",
					     "Moptop_Server_Connection_Callback",
					     LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Moptop_Command_Fits_Header(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
						     "Moptop_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Moptop_Command_Fits_Header failed.");
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
						     "Moptop_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strcmp(client_message, "help") == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				   LOG_VERBOSITY_VERY_TERSE,"SERVER","help detected.");
#endif
		/* normal thread priority */
		if(!Moptop_General_Thread_Priority_Set_Normal())
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		Send_Reply(connection_handle, "help:\n"
			   "\tabort\n"
			   "\tconfig filter <filter_name>\n"
			   "\tconfig bin <bin>\n"
			   "\tconfig rotorspeed <slow|fast>\n"
			   "\tfitsheader add <keyword> <boolean|float|integer|string> <value>\n"
			   "\tfitsheader delete <keyword>\n"
			   "\tfitsheader clear\n"
			   "\thelp\n"
/*			   "\tlog_level <moptop|ccd|command_server|object|ngatcil> <n>\n"*/
/*			   "\tmultdark <length> <count>\n"*/
			   "\tmultrun_setup\n"
			   "\tmultrun <length> <count> <standard>\n"
			   "\tstatus [name|identification|fits_instrument_code]\n"
			   "\tstatus temperature [get|status]\n"
			   "\tstatus filterwheel [filter|position|status]\n"
			   "\tstatus rotator [position|status]\n"
			   "\tstatus exposure [status|count|length|start_time|trigger_mode]\n"
			   "\tstatus exposure [accumulation|series|index|multrun|run|window]\n"
			   "\tshutdown\n");
	}
/*
	else if(strncmp(client_message,"multdark",7) == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","multdark detected.");
#endif
*/
		/* exposure thread priority */
/*
		if(!Moptop_General_Thread_Priority_Set_Exposure())
		{
			Moptop_General_Error("server","moptop_server.c","Moptop_Server_Connection_Callback",
					     LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Moptop_Command_Mult_Dark(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c","Moptop_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Moptop_General_Error("server","moptop_server.c","Moptop_Server_Connection_Callback",
					     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Moptop_Command_Mult_Dark failed.");
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c","Moptop_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
*/
	else if(strncmp(client_message,"multrun",7) == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","multrun detected.");
#endif
		/* exposure thread priority */
		if(!Moptop_General_Thread_Priority_Set_Exposure())
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Moptop_Command_Multrun(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Moptop_Command_Multrun failed.");
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strncmp(client_message,"multrun_setup",13) == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","multrun_setup detected.");
#endif
		/* exposure thread priority */
		if(!Moptop_General_Thread_Priority_Set_Exposure())
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Moptop_Command_Multrun_Setup(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Moptop_Command_Multrun_Setup failed.");
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strncmp(client_message,"status",6) == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","status detected.");
#endif
		/* normal thread priority */
		if(!Moptop_General_Thread_Priority_Set_Normal())
		{
			Moptop_General_Error("server","moptop_server.c","Moptop_Server_Connection_Callback",
			                     LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Moptop_Command_Status(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Moptop_Command_Status failed.");
			if(retval == FALSE)
			{
				Moptop_General_Error("server","moptop_server.c",
							 "Moptop_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strcmp(client_message, "shutdown") == 0)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("server","moptop_server.c","Moptop_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","shutdown detected:about to stop.");
#endif
		/* normal thread priority */
		if(!Moptop_General_Thread_Priority_Set_Normal())
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Send_Reply(connection_handle, "0 ok");
		if(retval == FALSE)
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		retval = Moptop_Server_Stop();
		if(retval == FALSE)
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
	}
	else
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log_Format("server","moptop_server.c","Moptop_Server_Connection_Callback",
					      LOG_VERBOSITY_VERY_TERSE,"SERVER","message unknown: '%s'\n",
					      client_message);
#endif
		retval = Send_Reply(connection_handle, "1 failed message unknown");
		if(retval == FALSE)
		{
			Moptop_General_Error("server","moptop_server.c",
						 "Moptop_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
	}
	/* free message */
	free(client_message);
}

/**
 * Send a message back to the client.
 * @param connection_handle Globus_io connection handle for this thread.
 * @param reply_message The message to send.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see ../command_server/cdocs/command_server.html#Command_Server_Write_Message
 */
static int Send_Reply(Command_Server_Handle_T connection_handle,char *reply_message)
{
	int retval;

	/* send something back to the client */
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("server","moptop_server.c","Send_Reply",LOG_VERBOSITY_TERSE,"SERVER",
				      "about to send '%.80s'...",reply_message);
#endif
	retval = Command_Server_Write_Message(connection_handle, reply_message);
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 204;
		sprintf(Moptop_General_Error_String,"Send_Reply:"
			"Writing message to connection failed.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("server","moptop_server.c","Send_Reply",LOG_VERBOSITY_TERSE,"SERVER",
				      "sent '%.80s'...",reply_message);
#endif
	return TRUE;
}

/**
 * Send a binary message back to the client.
 * @param connection_handle The connection handle for this thread.
 * @param buffer_ptr A pointer to the binary data to send.
 * @param buffer_length The number of bytes in the binary buffer.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see ../command_server/cdocs/command_server.html#Command_Server_Write_Binary_Message
 */
static int Send_Binary_Reply(Command_Server_Handle_T connection_handle,void *buffer_ptr,size_t buffer_length)
{
	int retval;

#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("server","moptop_server.c","Send_Binary_Reply",
				      LOG_VERBOSITY_INTERMEDIATE,"SERVER",
				      "about to send %ld bytes.",buffer_length);
#endif
	retval = Command_Server_Write_Binary_Message(connection_handle,buffer_ptr,buffer_length);
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 206;
		sprintf(Moptop_General_Error_String,"Send_Binary_Reply:"
			"Writing binary message of length %ld to connection failed.",buffer_length);
		return FALSE;
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("server","moptop_server.c","Send_Binary_Reply",
				      LOG_VERBOSITY_INTERMEDIATE,"SERVER","sent %ld bytes.",buffer_length);
#endif
	return TRUE;
}

/**
 * Send a binary message back to the client, after something went wrong to stops ending a FITS image back.
 * This involves putting the error string into a buffer and sending that back as the binary data, the client
 * end should realise this is not a FITS image!
 * @param connection_handle The connection handle for this thread.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Error_To_String
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see ../command_server/cdocs/command_server.html#Command_Server_Write_Binary_Message
 */
static int Send_Binary_Reply_Error(Command_Server_Handle_T connection_handle)
{
	char error_buff[1024];
	int retval;


	Moptop_General_Error_To_String("server","moptop_server.c","Send_Binary_Reply_Error",
				      LOG_VERBOSITY_INTERMEDIATE,"SERVER",error_buff);
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("server","moptop_server.c","Send_Binary_Reply_Error",
				      LOG_VERBOSITY_INTERMEDIATE,"SERVER",
				      "about to send error '%s' : Length %ld bytes.",
				      error_buff,strlen(error_buff));
#endif
	retval = Command_Server_Write_Binary_Message(connection_handle,error_buff,strlen(error_buff));
	if(retval == FALSE)
	{
		Moptop_General_Error_Number = 205;
		sprintf(Moptop_General_Error_String,"Send_Binary_Reply_Error:"
			"Writing binary error message '%s' of length %ld to connection failed.",
			error_buff,strlen(error_buff));
		return FALSE;
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("server","moptop_server.c","Send_Binary_Reply_Error",
				      LOG_VERBOSITY_INTERMEDIATE,"SERVER","sent %ld bytes.",strlen(error_buff));
#endif
	return TRUE;
}

