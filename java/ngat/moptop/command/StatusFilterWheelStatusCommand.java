// StatusFilterWheelStatusCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "status filterwheel status" command is an extension of Command, and returns whether the filter
 * wheel is currently in position or moving.
 * This status is available from moptop1 ony, on moptop2 the command will fail as the filter wheel is not connected.
 * @author Chris Mottram
 * @version $Revision$
 */
public class StatusFilterWheelStatusCommand extends Command implements Runnable
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
	 * This command returns the string "in_position" if the wheel is currently in a valid position.
	 */
	public final static String FILTER_WHEEL_IN_POSITION = new String("in_position");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status filterwheel status");
	/**
	 * The current filter wheel status, "moving" if the wheel was moving, and "in_position" if it is currently
	 * in a valid position.
	 * @see #FILTER_WHEEL_MOVING
	 * @see #FILTER_WHEEL_IN_POSITION
	 */
	protected String statusString = null;
	
	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusFilterWheelStatusCommand()
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
	public StatusFilterWheelStatusCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Parse a string returned from the server over the telnet connection.
	 * @exception Exception Thrown if a parse error occurs.
	 * @see #replyString
	 * @see #parsedReplyString
	 * @see #parsedReplyOk
	 * @see #statusString
	 */
	public void parseReplyString() throws Exception
	{
		super.parseReplyString();
		if(parsedReplyOk == false)
		{
			statusString = null;
			return;
		}
		statusString = parsedReplyString;
	}
	
	/**
	 * Get the current filter wheel status.
	 * @return The current filter wheel status as a string, "moving" or "in_position" 
	 * @see #statusString
	 * @see #FILTER_WHEEL_MOVING
	 * @see #FILTER_WHEEL_IN_POSITION
	 */
	public String getFilterWheelStatus()
	{
		return statusString;
	}
	
	/**
	 * Return whether the filter wheel is currently moving.
	 * @return Returns true if the statusString equals FILTER_WHEEL_MOVING, false if it is null or 
	 *         FILTER_WHEEL_IN_POSITION.
	 * @see #statusString
	 * @see #FILTER_WHEEL_MOVING
	 * @see #FILTER_WHEEL_IN_POSITION
	 */
	public boolean filterWheelIsMoving()
	{
		if(statusString == null)
			return false;
		return statusString.equals(FILTER_WHEEL_MOVING);
	}
	
	/**
	 * Return whether the filter wheel is currently in position.
	 * @return Returns true if the statusString equals FILTER_WHEEL_IN_POSITION, false if it is null or 
	 *         FILTER_WHEEL_MOVING.
	 * @see #statusString
	 * @see #FILTER_WHEEL_MOVING
	 * @see #FILTER_WHEEL_IN_POSITION
	 */
	public boolean filterWheelIsInPosition()
	{
		if(statusString == null)
			return false;
		return statusString.equals(FILTER_WHEEL_IN_POSITION);
	}
	
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusFilterWheelStatusCommand command = null;
		String hostname = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.StatusFilterWheelStatusCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];			
			portNumber = Integer.parseInt(args[1]);
			command = new StatusFilterWheelStatusCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusFilterWheelStatusCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Filter Wheel Status:"+command.getFilterWheelStatus());
			System.out.println("Filter Wheel Is Moving:"+command.filterWheelIsMoving());
			System.out.println("Filter Wheel Is In Position:"+command.filterWheelIsInPosition());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
