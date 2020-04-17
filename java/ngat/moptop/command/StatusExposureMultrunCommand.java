// StatusExposureMultrunCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;

/**
 * The "status exposure multrun" command is an extension of the IntegerReplyCommand, and returns the 
 * multrun number used in the FITS filenames for the current (or last) multrun.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class StatusExposureMultrunCommand extends IntegerReplyCommand implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status exposure multrun");

	/**
	 * Default constructor.
	 * @see IntegerReplyCommand
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusExposureMultrunCommand()
	{
		super();
		commandString = COMMAND_STRING;
	}

	/**
	 * Constructor.
	 * @param address A string representing the address of the server, i.e. "moptop1",
	 *     "localhost", "192.168.1.28"
	 * @param portNumber An integer representing the port number the server is receiving command on.
	 * @see IntegerReplyCommand
	 * @see #COMMAND_STRING
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public StatusExposureMultrunCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Get the current multrun number.
	 * @return An integer, the multrun number.
	 * @exception Exception Thrown if getting the data fails, either the run method failed to communicate
	 *         with the server in some way, or the method was called before the command had completed.
	 */
	public int getExposureMultrun() throws Exception
	{
		if(parsedReplyOk)
			return super.getParsedReplyInteger();
		else
		{
			if(runException != null)
				throw runException;
			else
				throw new Exception(this.getClass().getName()+":getExposureMultrun:Unknown Error.");
		}
	}

	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusExposureMultrunCommand command = null;
		String hostname = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.ringo2.command.StatusExposureMultrunCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			command = new StatusExposureMultrunCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusExposureMultrunCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Exposure Multrun number:"+command.getExposureMultrun());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
