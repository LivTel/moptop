// ConfigFilterCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "config" command is an extension of the Command, and configures the instrument for exposures.
 * This command configures the filter to be used.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class ConfigFilterCommand extends Command implements Runnable
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
	public ConfigFilterCommand()
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
	public ConfigFilterCommand(String address,int portNumber) throws UnknownHostException
	{
		super();
		super.setAddress(address);
		super.setPortNumber(portNumber);
	}

	/**
	 * Setup the command to send to the server.
	 * @param filterString A string representing the filter to be selected.
	 * @see #commandString
	 */
	public void setCommand(String filterString)
	{
		commandString = new String("config filter "+filterString);
	}
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		ConfigFilterCommand command = null;
		String hostname = null;
		String filterString = null;
		int portNumber = 1111;

		if(args.length != 3)
		{
			System.out.println("java ngat.moptop.command.ConfigFilterCommand <hostname> <port number> <filter>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			filterString = args[2];
			command = new ConfigFilterCommand(hostname,portNumber);
			command.setCommand(filterString);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("ConfigFilterCommand: Command failed.");
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
