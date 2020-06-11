// StatusRotatorStatusCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "status rotator status" command is an extension of Command, and returns whether the rotator
 * wheel is currently in position or moving.
 * This status is available from moptop1 ony, on moptop2 the command will fail as the rotator is not connected.
 * @author Chris Mottram
 * @version $Revision$
 */
public class StatusRotatorStatusCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * This command returns the string "moving" if the rotator was moving.
	 */
	public final static String ROTATOR_MOVING = new String("moving");
	/**
	 * This command returns the string "stopped" if the rotator is currently 'on target' i.e. not moving.
	 */
	public final static String ROTATOR_STOPPED = new String("stopped");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status rotator status");
	/**
	 * The current rotator status, "moving" if the rotator was moving, and "stopped" if it is
	 * in 'on target'/stopped.
	 * @see #ROTATOR_MOVING
	 * @see #ROTATOR_STOPPED
	 */
	protected String statusString = null;
	
	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusRotatorStatusCommand()
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
	public StatusRotatorStatusCommand(String address,int portNumber) throws UnknownHostException
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
	 * Get the current rotator status.
	 * @return The current rotator status as a string, "moving" or "stopped". 
	 * @see #statusString
	 * @see #ROTATOR_MOVING
	 * @see #ROTATOR_STOPPED
	 */
	public String getRotatorStatus()
	{
		return statusString;
	}
	
	/**
	 * Return whether the rotator is currently moving.
	 * @return Returns true if the statusString equals ROTATOR_MOVING, false if it is null or 
	 *         ROTATOR_STOPPED.
	 * @see #statusString
	 * @see #ROTATOR_MOVING
	 * @see #ROTATOR_STOPPED
	 */
	public boolean rotatorIsMoving()
	{
		if(statusString == null)
			return false;
		return statusString.equals(ROTATOR_MOVING);
	}
	
	/**
	 * Return whether the rotator is currently stopped.
	 * @return Returns true if the statusString equals ROTATOR_STOPPED, false if it is null or 
	 *         ROTATOR_MOVING.
	 * @see #statusString
	 * @see #ROTATOR_MOVING
	 * @see #ROTATOR_STOPPED
	 */
	public boolean rotatorIsStopped()
	{
		if(statusString == null)
			return false;
		return statusString.equals(ROTATOR_STOPPED);
	}
	
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusRotatorStatusCommand command = null;
		String hostname = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.StatusRotatorStatusCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];			
			portNumber = Integer.parseInt(args[1]);
			command = new StatusRotatorStatusCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusRotatorStatusCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Rotator Status:"+command.getRotatorStatus());
			System.out.println("Rotator Is Moving:"+command.rotatorIsMoving());
			System.out.println("Rotator Is In Stopped:"+command.rotatorIsStopped());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
