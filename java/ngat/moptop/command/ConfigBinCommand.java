// ConfigBinCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "config" command is an extension of the Command, and configures the instrument for exposures.
 * This command configures the CCD binning to be used.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class ConfigBinCommand extends Command implements Runnable
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
	public ConfigBinCommand()
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
	public ConfigBinCommand(String address,int portNumber) throws UnknownHostException
	{
		super();
		super.setAddress(address);
		super.setPortNumber(portNumber);
	}

	/**
	 * Setup the command to send to the server.
	 * @param binning An integer (from 1 to 4) representing the binning of the CCD.
	 * @see #commandString
	 */
	public void setCommand(int binning)
	{
		commandString = new String("config bin "+binning);
	}
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		ConfigBinCommand command = null;
		String hostname = null;
		int portNumber = 1111;
		int bin;
		
		if(args.length != 3)
		{
			System.out.println("java ngat.moptop.command.ConfigBinCommand <hostname> <port number> <bin>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			bin = Integer.parseInt(args[2]);
			command = new ConfigBinCommand(hostname,portNumber);
			command.setCommand(bin);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("ConfigBinCommand: Command failed.");
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
