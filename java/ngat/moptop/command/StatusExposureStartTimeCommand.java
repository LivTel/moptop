// StatusExposureStartTimeCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "status exposure start_time" command is an extension of the Command, and returns the 
 * start_time of the current/last exposure, as a timestamp.
 * This status is available per C layer.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class StatusExposureStartTimeCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status exposure start_time");
	/**
	 * The parsed reply timestamp.
	 */
	protected Date parsedReplyTimestamp = null;

	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusExposureStartTimeCommand()
	{
		super();
		commandString = COMMAND_STRING;
	}

	/**
	 * Constructor.
	 * @param address A string representing the address of the server, i.e. "moptop1",
	 *     "localhost", "192.168.1.28"
	 * @param portNumber An integer representing the port number the server is receiving command on.
	 * @see Command
	 * @see #COMMAND_STRING
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public StatusExposureStartTimeCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Parse a string returned from the server over the telnet connection.
	 * In this case it is of the form: '<n> %Y-%m-%dT%H:%M:%S.sss ZZZ'
	 * The first number is a success failure code, if it is zero a timestamp follows.
	 * We ignore the timezone at the moment.
	 * @exception Exception Thrown if a parse error occurs.
	 * @see #replyString
	 * @see #parsedReplyString
	 * @see #parsedReplyOk
	 * @see #parsedReplyTimestamp
	 */
	public void parseReplyString() throws Exception
	{
		Calendar calendar = null;
		StringTokenizer st = null;
		String timeStampDateString = null;
		String timeStampTimeString = null;
		String temperatureString = null;
		double second=0.0;
		int sindex,tokenIndex,day=0,month=0,year=0,hour=0,minute=0;
		
		super.parseReplyString();
		if(parsedReplyOk == false)
		{
			parsedReplyTimestamp = null;
			return;
		}
		st = new StringTokenizer(parsedReplyString,"T");
		tokenIndex = 0;
		while(st.hasMoreTokens())
		{
			if(tokenIndex == 0)
				timeStampDateString = st.nextToken();
			else if(tokenIndex == 1)
				timeStampTimeString = st.nextToken();
			else
				st.nextToken();
			tokenIndex++;
		}// end while
		// timeStampDateString should be of the form: %Y-%m-%d
		st = new StringTokenizer(timeStampDateString,"-");
		tokenIndex = 0;
		while(st.hasMoreTokens())
		{
			if(tokenIndex == 0)
				year = Integer.parseInt(st.nextToken());// year including century
			else if(tokenIndex == 1)
				month = Integer.parseInt(st.nextToken());// 01..12
			else
				day = Integer.parseInt(st.nextToken());// 0..31
			tokenIndex++;
		}// end while
		// timeStampTimeString should be of the form: %H:%M:%S.sss
		st = new StringTokenizer(timeStampTimeString,":");
		tokenIndex = 0;
		while(st.hasMoreTokens())
		{
			if(tokenIndex == 0)
				hour = Integer.parseInt(st.nextToken());// 0..23
			else if(tokenIndex == 1)
				minute = Integer.parseInt(st.nextToken());// 00..59
			else
				second = Double.parseDouble(st.nextToken());// 00..61 + milliseconds as decimal
			tokenIndex++;
		}// end while
		// create calendar
		calendar = Calendar.getInstance();
		// set calendar
		calendar.set(year,month-1,day,hour,minute,(int)second);// month is zero-based.
		// get timestamp from calendar 
		parsedReplyTimestamp = calendar.getTime();
	}

	/**
	 * Get the timestamp representing the start time of the current/last 
	 * exposure.
	 * @return A date.
	 * @exception Exception Thrown if getting the data fails, either the run method failed to communicate
	 *         with the server in some way, or the method was called before the command had completed.
	 * @see #parsedReplyOk
	 * @see #runException
	 * @see #parsedReplyTimestamp
	 */
	public Date getTimestamp() throws Exception
	{
		if(parsedReplyOk)
		{
			return parsedReplyTimestamp;
		}
		else
		{
			if(runException != null)
				throw runException;
			else
				throw new Exception(this.getClass().getName()+":getTimestamp:Unknown Error.");
		}
	}

	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusExposureStartTimeCommand command = null;
		String hostname = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.StatusExposureStartTimeCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];			
			portNumber = Integer.parseInt(args[1]);
			command = new StatusExposureStartTimeCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusExposureStartTimeCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Start Time stamp:"+command.getTimestamp());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
