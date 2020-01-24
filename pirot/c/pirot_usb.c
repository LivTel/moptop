/* pirot_usb.c
** PI Rotator library USB interface routines
** $Header$
*/
/**
 * USB routines to open and close a connection to the PI rotator, using USB and PI's library.
 * @author Chris Mottram
 * @version $Revision$
 */
#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PI_GCS2_DLL.h"
#include "log_udp.h"
#include "pirot_general.h"
#include "pirot_usb.h"

/* hash defines */
/**
 * The default number of times to attempt to connect to the PI rotator, before we give up and declare a failure.
 */
#define DEFAULT_CONNECTION_COUNT    (5)

/* structures */
/**
 * Structure holding local data pertinent to the USB module. This consists of:
 * <ul>
 * <li><b>Connection_Count</b> The number of times to attempt to connect to the rotator before we fail.
 * <li><b>Id</b> The PI library ID returned by a successful connection to the library, and therefore the rotator.
 * </ul>
 */
struct USB_Struct
{
	int Connection_Count;
	int Id;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Variable holding error code of last operation performed.
 */
static int USB_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see #PIROT_ERROR_STRING_LENGTH
 */
static char USB_Error_String[PIROT_ERROR_STRING_LENGTH] = "";
/**
 * Instance of local data.
 * @see #USB_Struct
 */
static struct USB_Struct USB_Data = {DEFAULT_CONNECTION_COUNT,-1};

/* --------------------------------------------
** external functions 
** -------------------------------------------- */
/**
 * Routine to open the conenction to the PI rotator via a USB connection.
 * The opened connection PI library routine returns an ID, that is stored internally in the
 * modules USB_Data.
 * We attempt to make the connection several times, as it sometimes fails a few times after a reboot.
 * We also turn of the library automatic error checking. This increases communication speeds (i.e. no "ERR?" command
 * sent after every command).
 * @param device_name A NULL terminated string denoting which device to conenct to, i.e. "/dev/ttyUSB0".
 * @param baud_rate The baud rate to use over this connection.
 * @return The routine returns TRUE on success, and FALSE if it fails. If it fails, the USB error number and string are
 *        filled in with a reason for the failure. If it succeeds, the connection's returned ID is stored in the
 *        USB_Data.
 * @see #USB_Data
 * @see #USB_Struct
 * @see #USB_Error_Number
 * @see #USB_Error_String
 */
int PIROT_USB_Open(char *device_name,int baud_rate)
{
	int connection_count;

	if(device_name == NULL)
	{
		USB_Error_Number = 1;
		sprintf(USB_Error_String,"PIROT_USB_Open: device_name was NULL.");
		return FALSE;
	}
	/* Have several attempts to connect to the rotator. The first few attempts after a reboot can fail */
	USB_Data.Id = -1;
	connection_count = 0;
	if(!PIROT_Mutex_Lock())
	{
		USB_Error_Number = 2;
		sprintf(USB_Error_String,"PIROT_USB_Open: Failed to obtain the mutex.");
		return FALSE;
	}
	while((USB_Data.Id < 0)&&(connection_count < USB_Data.Connection_Count))
	{
#if LOGGING > 0
		PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_USB_Open(%s,%d): Connection attempt %d.",
				 device_name,baud_rate,connection_count+1);
#endif /* LOGGING */
		USB_Data.Id = PI_ConnectRS232ByDevName(device_name,baud_rate);
		if(USB_Data.Id < 0)
		{
			USB_Error_Number = 3;
			sprintf(USB_Error_String,"PIROT_USB_Open: PI_ConnectRS232ByDevName failed (%d).",USB_Data.Id);
			PIROT_USB_Error();
			/* do not return FALSE, but retry (as part of the loop). */
		}
		/* sleep a while before retrying */
		sleep(1);
		connection_count++;
	}/* end while */
	if(!PIROT_Mutex_Unlock())
	{
		USB_Error_Number = 4;
		sprintf(USB_Error_String,"PIROT_USB_Open: Failed to release the mutex.");
		return FALSE;
	}
	/* if we have successfully connected the Id will be greater than -1 */
	if(USB_Data.Id < 0)
	{
		USB_Error_Number = 5;
		sprintf(USB_Error_String,"PIROT_USB_Open: PI_ConnectRS232ByDevName failed %d times (%d).",
			connection_count,USB_Data.Id);
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_USB_Open(%s,%d): Connection returned ID %d.",
			 device_name,baud_rate,USB_Data.Id);
#endif /* LOGGING */
	/* Turn internal PI library error checking off.
	** This means we should call PI_GetError to explicitly do error checking.
	** This speeds up function calls, which may be useful in certain cases */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,
			 "PIROT_USB_Open: Turning internal library error checking off (PI_SetErrorCheck).");
#endif /* LOGGING */
	PI_SetErrorCheck(USB_Data.Id,FALSE);
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_USB_Open(%s,%d): Finished.",device_name,baud_rate);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to close an open connection to the PI rotator, using PI_CloseConnection with the Id in USB_Data.
 * @return PI_CloseConnection has no failure mode, so the routine always returns  TRUE.
 * @see #USB_Data
 */
int PIROT_USB_Close(void)
{
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_USB_Close: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_USB_Close: Calling PI_CloseConnection(id=%d).",USB_Data.Id);
#endif /* LOGGING */
	PI_CloseConnection(USB_Data.Id);
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_USB_Close: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * This routine returns the PI rotator library ID return by the open call in PIROT_USB_Open.
 * @return The PI rotator library Id.
 * @see #USB_Data
 */
int PIROT_USB_Get_ID(void)
{
	return USB_Data.Id;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #USB_Error_Number
 */
int PIROT_USB_Get_Error_Number(void)
{
	return USB_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 * @see #USB_Error_Number
 * @see #USB_Error_String
 */
void PIROT_USB_Error(void)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(USB_Error_Number == 0)
		sprintf(USB_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s PIROT_USB:Error(%d) : %s\n",time_string,USB_Error_Number,USB_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #USB_Error_Number
 * @see #USB_Error_String
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 */
void PIROT_USB_Error_String(char *error_string)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(USB_Error_Number == 0)
		sprintf(USB_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s PIROT_USB:Error(%d) : %s\n",time_string,
		USB_Error_Number,USB_Error_String);
}

