// MultbiasCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "multbias" command is an extension of the Command, and takes a series of exposures.
 * @author Chris Mottram
 * @version $Revision$
 */
public class MultbiasCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 */
	public MultbiasCommand()
	{
		super();
		commandString = null;
	}

	/**
	 * Constructor.
	 * @param address A string representing the address of the server, i.e. "moptop1",
	 *     "localhost", "192.168.1.28"
	 * @param portNumber An integer representing the port number the server is receiving command on.
	 * @see Command
	 * @see Command#setAddress
	 * @see Command#setPortNumber
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public MultbiasCommand(String address,int portNumber) throws UnknownHostException
	{
		super();
		super.setAddress(address);
		super.setPortNumber(portNumber);
	}

	/**
	 * Setup the Multbias command. 
	 * @param exposureCount Set the number of frames to take in the Multbias. This should be greater than zero.
	 * @see #commandString
	 */
	public void setCommand(int exposureCount)
	{
		commandString = new String("multbias "+exposureCount);
	}

	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		MultbiasCommand command = null;
		String hostname = null;
		int portNumber = 1111;
		int exposureCount;

		if(args.length != 3)
		{
			System.out.println("java ngat.moptop.command.MultbiasCommand <hostname> <port number> <exposure count>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			exposureCount = Integer.parseInt(args[2]);
			command = new MultbiasCommand(hostname,portNumber);
			command.setCommand(exposureCount);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("MultbiasCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Return Code:"+command.getReturnCode());
			System.out.println("Reply String:"+command.getParsedReply());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
