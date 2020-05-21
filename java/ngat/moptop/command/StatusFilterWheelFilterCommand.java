// StatusFilterWheelFilterCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "status filterwheel filter" command is an extension of Command, and returns the name of the
 * currently selected filter in the filter wheel, or "moving" if the wheel is currently moving.
 * This status is available from moptop1 ony, on moptop2 the command will fail as the filter wheel is not connected.
 * @author Chris Mottram
 * @version $Revision$
 */
public class StatusFilterWheelFilterCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * This command returns the string "moving" if the wheel was moving.
	 */
	public final static String FILTER_WHEEL_MOVING = new String("moving");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status filterwheel filter");
	/**
	 * The name of the selected filter in the filter wheel, or "moving" if the wheel was moving.
	 */
	protected String filterName = null;
	
	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusFilterWheelFilterCommand()
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
	public StatusFilterWheelFilterCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Parse a string returned from the server over the telnet connection.
	 * @exception Exception Thrown if a parse error occurs.
	 * @see #replyString
	 * @see #parsedReplyString
	 * @see #parsedReplyOk
	 * @see #filterName
	 */
	public void parseReplyString() throws Exception
	{
		super.parseReplyString();
		if(parsedReplyOk == false)
		{
			filterName = null;
			return;
		}
		filterName = parsedReplyString;
	}
	
	/**
	 * Get the currently selected filter name.
	 * @return The currently selected filter as a string, or "moving" if the filter wheel is currently moving.
	 * @see #filterName
	 */
	public String getFilterName()
	{
		return filterName;
	}
	
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusFilterWheelFilterCommand command = null;
		String hostname = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.StatusFilterWheelFilterCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];			
			portNumber = Integer.parseInt(args[1]);
			command = new StatusFilterWheelFilterCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusFilterWheelFilterCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Filter name:"+command.getFilterName());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
