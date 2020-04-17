// ConfigRotorspeedCommand.java
// $Id$
package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "config" command is an extension of the Command, and configures the instrument for exposures.
 * This command configures the rotator speed to be used.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class ConfigRotorspeedCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * String to be used when selecting a "slow" rotorspeed.
	 */
	public final static String ROTORSPEED_SLOW = new String("slow");
	/**
	 * String to be used when selecting a "fast" rotorspeed.
	 */
	public final static String ROTORSPEED_FAST = new String("fast");

	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 */
	public ConfigRotorspeedCommand()
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
	public ConfigRotorspeedCommand(String address,int portNumber) throws UnknownHostException
	{
		super();
		super.setAddress(address);
		super.setPortNumber(portNumber);
	}

	/**
	 * Setup the command to send to the server.
	 * @param rotorspeedString A string used to configure the rotor speed, one of: 
	 *        ROTORSPEED_SLOW or ROTORSPEED_FAST.
	 * @see #commandString
	 * @see #ROTORSPEED_SLOW
	 * @see #ROTORSPEED_FAST
	 */
	public void setCommand(String rotorspeedString)
	{
		commandString = new String("config rotospeed "+rotorspeedString);
	}
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		ConfigRotorspeedCommand command = null;
		String hostname = null;
		String rotorspeedString = null;
		int portNumber = 1111;

		if(args.length != 3)
		{
			System.out.println("java ngat.moptop.command.ConfigRotorspeedCommand <hostname> <port number> <slow|fast>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			rotorspeedString = args[2];
			command = new ConfigRotorspeedCommand(hostname,portNumber);
			command.setCommand(rotorspeedString);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("ConfigRotorspeedCommand: Command failed.");
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
