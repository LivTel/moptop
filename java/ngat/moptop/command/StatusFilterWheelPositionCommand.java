// StatusFilterWheelPositionCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;

/**
 * The "status filterwheel position" command is an extension of the IntegerReplyCommand, and returns the 
 * selected filter index position in the filter wheel, or 0 if the filter wheel is moving.
 * @author Chris Mottram
 * @version $Revision$
 */
public class StatusFilterWheelPositionCommand extends IntegerReplyCommand implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * This command returns 0 if the wheel was moving.
	 */
	public final static int FILTER_WHEEL_MOVING = 0;
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status filterwheel position");
	/**
	 * The position of the selected filter in the filter wheel, or 0 if the wheel was moving.
	 */
	protected int filterPosition = -1;
	
	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusFilterWheelPositionCommand()
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
	public StatusFilterWheelPositionCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Get the current filter wheel position.
	 * @return An integer, the current filter wheel position, or 0 if the wheel was moving.
	 * @exception Exception Thrown if getting the data fails, either the run method failed to communicate
	 *         with the server in some way, or the method was called before the command had completed.
	 */
	public int getFilterWheelPosition() throws Exception
	{
		if(parsedReplyOk)
			return super.getParsedReplyInteger();
		else
		{
			if(runException != null)
				throw runException;
			else
				throw new Exception(this.getClass().getName()+":getFilterWheelPosition:Unknown Error.");
		}
	}

	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusFilterWheelPositionCommand command = null;
		String hostname = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.StatusFilterWheelPositionCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];			
			portNumber = Integer.parseInt(args[1]);
			command = new StatusFilterWheelPositionCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusFilterWheelPositionCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Filter position:"+command.getFilterWheelPosition());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
