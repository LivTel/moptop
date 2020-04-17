// FitsHeaderClearCommand.java
// $Id$

package ngat.moptop.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "fitsheader clear" command is an extension of the Command, and 
 * clears the servers list of FITS header keywords to add to the FITS header of saved images.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class FitsHeaderClearCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("fitsheader clear");

	/**
	 * Default constructor.
	 * @see Command
	 * @see #COMMAND_STRING
	 */
	public FitsHeaderClearCommand()
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
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public FitsHeaderClearCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		FitsHeaderClearCommand command = null;
		int portNumber = 1111;

		if(args.length != 2)
		{
			System.out.println("java ngat.moptop.command.FitsHeaderClearCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			portNumber = Integer.parseInt(args[1]);
			command = new FitsHeaderClearCommand(args[0],portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("FitsHeaderClearCommand: Command failed.");
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
