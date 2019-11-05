// MoptopTCPServer.java
// $Header$
package ngat.moptop;

import java.lang.*;
import java.io.*;
import java.net.*;

import ngat.net.*;

/**
 * This class extends the TCPServer class for the Moptop application.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class MoptopTCPServer extends TCPServer
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Field holding the instance of Moptop currently executing, so we can pass this to spawned threads.
	 */
	private Moptop moptop = null;

	/**
	 * The constructor.
	 */
	public MoptopTCPServer(String name,int portNumber)
	{
		super(name,portNumber);
	}

	/**
	 * Routine to set this objects pointer to the moptop object.
	 * @param o The moptop object.
	 */
	public void setMoptop(Moptop o)
	{
		this.moptop = o;
	}

	/**
	 * This routine spawns threads to handle connection to the server. This routine
	 * spawns MoptopTCPServerConnectionThread threads.
	 * The routine also sets the new threads priority to higher than normal. This makes the thread
	 * reading it's command a priority so we can quickly determine whether the thread should
	 * continue to execute at a higher priority.
	 * @see MoptopTCPServerConnectionThread
	 */
	public void startConnectionThread(Socket connectionSocket)
	{
		MoptopTCPServerConnectionThread thread = null;

		thread = new MoptopTCPServerConnectionThread(connectionSocket);
		thread.setMoptop(moptop);
		thread.setPriority(moptop.getStatus().getThreadPriorityInterrupt());
		thread.start();
	}
}
//
// $Log$
//
