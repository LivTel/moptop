// MultrunSetupCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;

/**
 * The "multrun_setup" command checks the rotator is in the right start position, sorts out a new multrun number,
 * caches some data needed during the multrun. The routine returns the multrun number to be used (in the FITS
 * filename generation) so both C layers can be checked to ensure they are on the same multrun. For this reason we 
 * extend the IntegerReplyCommand.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class MultrunSetupCommand extends IntegerReplyCommand implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("multrun_setup");

	/**
	 * Default constructor.
	 * @see IntegerReplyCommand
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public MultrunSetupCommand()
	{
		super();
		commandString = COMMAND_STRING;
	}

	/**
	 * Constructor.
	 * @param address A string representing the address of the server, i.e. "moptop",
	 *     "localhost", "192.168.1.28"
	 * @param portNumber An integer representing the port number the server is receiving command on.
	 * @see IntegerReplyCommand
	 * @see #COMMAND_STRING
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public MultrunSetupCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Get the current multrun number.
	 * @return An integer, the multrun number.
	 * @exception Exception Thrown if getting the data fails, either the run method failed to communicate
	 *         with the server in some way, or the method was called before the command had completed.
	 */
	public int getMultrunNumber() throws Exception
	{
		if(parsedReplyOk)
			return super.getParsedReplyInteger();
		else
		{
			if(runException != null)
				throw runException;
			else
				throw new Exception(this.getClass().getName()+":getMultrunNumber:Unknown Error.");
		}
	}

	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		MultrunSetupCommand command = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.MultrunSetupCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			portNumber = Integer.parseInt(args[1]);
			command = new MultrunSetupCommand(args[0],portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("MultrunSetupCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Multrun number:"+command.getMultrunNumber());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
