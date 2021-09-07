// ABORTImplementation.java
// $Id$
package ngat.moptop;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.moptop.command.*;
import ngat.message.base.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the ABORT command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class ABORTImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The number of C layers we have to communicate with.
	 */
	protected int cLayerCount;
	/**
	 * A list of hostname strings, one per C layer, to send C layer commands to.
	 */
	protected Vector cLayerHostnameList = null;
	/**
	 * A list of port numbers, one per C layer, to send C layer commands to.
	 */
	protected Vector cLayerPortNumberList = null;

	/**
	 * Constructor.
	 */
	public ABORTImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.ABORT&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.ABORT";
	}

	/**
	 * This method gets the ABORT command's acknowledge time. This takes the default acknowledge time to implement.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the ABORT command. 
	 * <ul>
	 * <li>We call getCLayerConfig to get information about the C Layers we have to send the command to.
	 * <li>For each C layer, we send an abort command using sendAbortCommand.
	 * <li>We get the current command thread using status.getCurrentThread.
	 * <li>If the current thread is non-null, we call setAbortProcessCommand to tell the thread it is 
	 *     being aborted. When the command implemented in the running thread next calls 'testAbort' this will
	 *     inform the command it has been aborted.
	 * </ul>
	 * @param command The abort command.
	 * @return An object of class ABORT_DONE is returned.
	 * @see MoptopStatus#getCurrentThread
	 * @see MoptopTCPServerConnectionThread#setAbortProcessCommand
	 * @see #sendAbortCommand
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see #moptop
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		ABORT_DONE abortDone = new ABORT_DONE(command.getId());
		MoptopTCPServerConnectionThread thread = null;
		String cLayerHostname = null;
		int cLayerPortNumber;

		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		try
		{
			// get C layer comms configuration
			getCLayerConfig();
			// loop over C layer indexes
			for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
			{
				cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
				cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
				sendAbortCommand(cLayerHostname,cLayerPortNumber);
			}
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+":Aborting exposure failed:",e);
			abortDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+2400);
			abortDone.setErrorString(e.toString());
			abortDone.setSuccessful(false);
			return abortDone;
		}
	// tell the thread itself to abort at a suitable point
		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Tell thread to abort.");
		thread = (MoptopTCPServerConnectionThread)status.getCurrentThread();
		if(thread != null)
			thread.setAbortProcessCommand();
	// return done object.
		moptop.log(Logging.VERBOSITY_VERY_TERSE,"Command:"+command.getClass().getName()+
			  ":Abort command completed.");
		abortDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		abortDone.setErrorString("");
		abortDone.setSuccessful(true);
		return abortDone;
	}

	/**
	 * Get the C layer hostname and port number.
	 * @exception Exception Thrown if the relevant property can be received.
	 * @see #status
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see MoptopStatus#getProperty
	 * @see MoptopStatus#getPropertyInteger
	 */
	protected void getCLayerConfig() throws Exception
	{
		String s = null;
		int i;

		cLayerCount = status.getPropertyInteger("moptop.c.count");
		cLayerHostnameList = new Vector(cLayerCount);
		cLayerPortNumberList = new Vector(cLayerCount);
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			s = status.getProperty("moptop.c.hostname."+cLayerIndex);
			i = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			cLayerHostnameList.add(cLayerIndex,s);
			cLayerPortNumberList.add(cLayerIndex,new Integer(i));
		}
	}

	/**
	 * Send an abort command to a C layer.
	 * @param hostname The hostname of the machine the C Layer is running on.
	 * @param portNumber The port number the C layer is running on.
	 * @exception Exception Thrown if an error occurs.
	 * @see #status
	 * @see MoptopStatus#getProperty
	 * @see MoptopStatus#getPropertyInteger
	 * @see ngat.moptop.command.AbortCommand
	 * @see ngat.moptop.command.AbortCommand#setAddress
	 * @see ngat.moptop.command.AbortCommand#setPortNumber
	 * @see ngat.moptop.command.AbortCommand#sendCommand
	 * @see ngat.moptop.command.AbortCommand#getParsedReplyOK
	 * @see ngat.moptop.command.AbortCommand#getReturnCode
	 * @see ngat.moptop.command.AbortCommand#getParsedReply
	 */
	protected void sendAbortCommand(String hostname,int portNumber) throws Exception
	{
		AbortCommand command = null;
		int returnCode;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendAbortCommand:Started.");
		command = new AbortCommand();
		// configure C comms
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendAbortCommand:hostname = "+hostname+" :port number = "+
			   portNumber+".");
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,"sendAbortCommand:abort command sent to hostname:"+hostname+
				   " :port number:"+portNumber+" failed with return code "+returnCode+
				   " and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendAbortCommand:Command sent to hostname:"+hostname+
					    " :port number:"+portNumber+" failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendAbortCommand:Finished.");
	}
}
