// StatusTemperatureGetCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "status temperature get" command is an extension of the Command, and returns the 
 * current temperature, and a timestamp stating when the temperature was measured.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class StatusTemperatureGetCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status temperature get");
	/**
	 * The parsed reply timestamp.
	 */
	protected Date parsedReplyTimestamp = null;
	/**
	 * The parsed reply temperature (in degrees C?).
	 */
	protected double parsedReplyTemperature = 0.0;

	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusTemperatureGetCommand()
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
	public StatusTemperatureGetCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Parse a string returned from the server over the telnet connection.
	 * In this case it is of the form: '&lt;n&gt; &lt;YYYY-mm-ddTHH:MM:SS.sss&gt; &lt;TZ&gt; &lt;n.nnn&gt;'
	 * The first number is a success failure code, if it is zero a timestamp and temperature follows.
	 * @exception Exception Thrown if a parse error occurs.
	 * @see #replyString
	 * @see #parsedReplyString
	 * @see #parsedReplyOk
	 * @see #parsedReplyTimestamp
	 * @see #parsedReplyTemperature
	 */
	public void parseReplyString() throws Exception
	{
		Calendar calendar = null;
		TimeZone timeZone = null;
		StringTokenizer st = null;
		String timeStampDateTimeString = null;
		String timeStampTimeZoneString = null;
		String temperatureString = null;
		double second=0.0;
		int sindex,tokenIndex,day=0,month=0,year=0,hour=0,minute=0;
	
		super.parseReplyString();
		if(parsedReplyOk == false)
		{
			parsedReplyTimestamp = null;
			parsedReplyTemperature = 0.0;
			return;
		}
		st = new StringTokenizer(parsedReplyString," ");
		tokenIndex = 0;
		while(st.hasMoreTokens())
		{
			if(tokenIndex == 0)
				timeStampDateTimeString = st.nextToken();
			else if(tokenIndex == 1)
				timeStampTimeZoneString = st.nextToken();
			else if(tokenIndex == 2)
				temperatureString = st.nextToken();
			tokenIndex++;
		}// end while
		// timeStampDateTimeString should be of the form: YYYY-mm-ddTHH:MM:SS.sss
		st = new StringTokenizer(timeStampDateTimeString,"-T:");
		tokenIndex = 0;
		while(st.hasMoreTokens())
		{
			if(tokenIndex == 0)
				year = Integer.parseInt(st.nextToken());// year including century
			else if(tokenIndex == 1)
				month = Integer.parseInt(st.nextToken());// 01..12
			else if(tokenIndex == 2)
				day = Integer.parseInt(st.nextToken());// 0..31
			else if(tokenIndex == 3)
				hour = Integer.parseInt(st.nextToken());// 0..23
			else if(tokenIndex == 4)
				minute = Integer.parseInt(st.nextToken());// 00..59
			else if(tokenIndex == 5)
				second = Double.parseDouble(st.nextToken());// 00..61 + milliseconds as decimal
			tokenIndex++;
		}// end while
		// parse the timezone string timeStampTimezoneString
		timeZone = TimeZone.getTimeZone(timeStampTimeZoneString);
		// create calendar
		calendar = Calendar.getInstance();
		calendar.setTimeZone(timeZone);
		// set calendar
		calendar.set(year,month-1,day,hour,minute,(int)second);// month is zero-based.
		// get timestamp from calendar 
		parsedReplyTimestamp = calendar.getTime();
		// parse temperature
		parsedReplyTemperature = Double.parseDouble(temperatureString);
	}

	/**
	 * Get the temperature of the CCD at the specified timestamp.
	 * @return A double, the temperature (in degrees centigrade?).
	 * @exception Exception Thrown if getting the data fails, either the run method failed to communicate
	 *         with the server in some way, or the method was called before the command had completed.
	 * @see #parsedReplyOk
	 * @see #runException
	 * @see #parsedReplyTemperature
	 */
	public double getTemperature() throws Exception
	{
		if(parsedReplyOk)
		{
			return parsedReplyTemperature;
		}
		else
		{
			if(runException != null)
				throw runException;
			else
				throw new Exception(this.getClass().getName()+":getTemperature:Unknown Error.");
		}
	}

	/**
	 * Get the timestamp representing the time the temperature of the CCD was measured.
	 * @return A date, the time the temperature of the CCD was measured.
	 * @exception Exception Thrown if getting the data fails, either the run method failed to communicate
	 *         with the server in some way, or the method was called before the command had completed.
	 * @see #parsedReplyOk
	 * @see #runException
	 * @see #parsedReplyTemperature
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
		StatusTemperatureGetCommand command = null;
		String hostname = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.StatusTemperatureGetCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			command = new StatusTemperatureGetCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusTemperatureGetCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Temperature:"+command.getTemperature());
			System.out.println("At Timestamp:"+command.getTimestamp());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
