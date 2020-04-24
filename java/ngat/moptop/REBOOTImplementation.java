// REBOOTImplementation.java
// $Id$
package ngat.moptop;

import java.lang.*;
import java.io.IOException;
import java.util.*;

import ngat.message.base.*;
import ngat.message.ISS_INST.REBOOT;
import ngat.moptop.command.*;
import ngat.util.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the REBOOT command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class REBOOTImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Class constant used in calculating acknowledge times, when the acknowledge time connot be found in the
	 * configuration file.
	 */
	public final static int DEFAULT_ACKNOWLEDGE_TIME = 		300000;
	/**
	 * String representing the root part of the property key used to get the acknowledge time for 
	 * a certain level of reboot.
	 */
	public final static String ACK_TIME_PROPERTY_KEY_ROOT =	    "moptop.reboot.acknowledge_time.";
	/**
	 * String representing the root part of the property key used to decide whether a certain level of reboot
	 * is enabled.
	 */
	public final static String ENABLE_PROPERTY_KEY_ROOT =       "moptop.reboot.enable.";
	/**
	 * Set of constant strings representing levels of reboot. The levels currently start at 1, so index
	 * 0 is currently "NONE". These strings need to be kept in line with level constants defined in
	 * ngat.message.ISS_INST.REBOOT.
	 */
	public final static String REBOOT_LEVEL_LIST[] =  {"NONE","REDATUM","SOFTWARE","HARDWARE","POWER_OFF"};
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
	public REBOOTImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.REBOOT&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.REBOOT";
	}

	/**
	 * This method gets the REBOOT command's acknowledge time. This time is dependant on the level.
	 * This is calculated as follows:
	 * <ul>
	 * <li>If the level is LEVEL_REDATUM, the number stored in &quot; 
	 * moptop.reboot.acknowledge_time.REDATUM &quot; in the Moptop properties file is the timeToComplete.
	 * <li>If the level is LEVEL_SOFTWARE, the number stored in &quot; 
	 * moptop.reboot.acknowledge_time.SOFTWARE &quot; in the Moptop properties file is the timeToComplete.
	 * <li>If the level is LEVEL_HARDWARE, the number stored in &quot; 
	 * moptop.reboot.acknowledge_time.HARDWARE &quot; in the Moptop properties file is the timeToComplete.
	 * <li>If the level is LEVEL_POWER_OFF, the number stored in &quot; 
	 * moptop.reboot.acknowledge_time.POWER_OFF &quot; in the Moptop properties file is the timeToComplete.
	 * </ul>
	 * If these numbers cannot be found, the default number DEFAULT_ACKNOWLEDGE_TIME is used instead.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set to a time (in milliseconds).
	 * @see #DEFAULT_ACKNOWLEDGE_TIME
	 * @see #ACK_TIME_PROPERTY_KEY_ROOT
	 * @see #REBOOT_LEVEL_LIST
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopStatus#getPropertyInteger
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ngat.message.ISS_INST.REBOOT rebootCommand = (ngat.message.ISS_INST.REBOOT)command;
		ACK acknowledge = null;
		int timeToComplete = 0;

		acknowledge = new ACK(command.getId()); 
		try
		{
			timeToComplete = status.getPropertyInteger(ACK_TIME_PROPERTY_KEY_ROOT+
								   REBOOT_LEVEL_LIST[rebootCommand.getLevel()]);
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+":calculateAcknowledgeTime:"+
				     rebootCommand.getLevel(),e);
			timeToComplete = DEFAULT_ACKNOWLEDGE_TIME;
		}
	//set time and return
		acknowledge.setTimeToComplete(timeToComplete);
		return acknowledge;
	}

	/**
	 * This method implements the REBOOT command. 
	 * An object of class REBOOT_DONE is returned.
	 * The <i>moptop.reboot.enable.&lt;level&gt;</i> property is checked to see to whether to really
	 * do the specified level of reboot. This enables us to say, disbale to POWER_OFF reboot, if the
	 * instrument control computer is not connected to an addressable power supply.
	 * The following four levels of reboot are recognised:
	 * <ul>
	 * <li>REDATUM. This shuts down the connection to the controller, and then
	 * 	restarts it. 
	 * <li>SOFTWARE. This sends the "shutdown" command to the C layer, which stops 
	 *      (and via the autobooter, restarts) the C layer software. It then closes the
	 * 	server socket using the Moptop close method. It then exits the Moptop control software.
	 * <li>HARDWARE. This shuts down the connection to the Apogee CCD Controller and closes the
	 * 	server socket using the Moptop close method. It then issues a reboot
	 * 	command to the underlying operating system, to restart the instrument computer.
	 * <li>POWER_OFF. This closes the
	 * 	server socket using the Moptop close method. It should then issue a shutdown
	 * 	command to the underlying operating system, to put the instrument computer into a state
	 * 	where power can be switched off. Currently the shutdown is not issused, as there is no way to
	 *      tell the C layer to warm the CCD up, and as this may be powered from the control computer
	 *      it may damage the CCD by powering down the control computer.
	 * </ul>
	 * Note: You need to perform at least a SOFTWARE level reboot to re-read the Moptop configuration file,
	 * as it contains information such as server ports.
	 * @param command The command instance we are implementing.
	 * @return An instance of REBOOT_DONE. Note this is only returned on a REDATUM level reboot,
	 *         all other levels cause the Moptop to terminate (either directly or indirectly) and a DONE
	 *         message cannot be returned.
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_REDATUM
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_SOFTWARE
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_HARDWARE
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_POWER_OFF
	 * @see #ENABLE_PROPERTY_KEY_ROOT
	 * @see #REBOOT_LEVEL_LIST
	 * @see #getCLayerConfig
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see #sendShutdownCommand
	 * @see Moptop#close
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		ngat.message.ISS_INST.REBOOT rebootCommand = (ngat.message.ISS_INST.REBOOT)command;
		ngat.message.ISS_INST.REBOOT_DONE rebootDone = new ngat.message.ISS_INST.REBOOT_DONE(command.getId());
		ngat.message.INST_DP.REBOOT dprtReboot = new ngat.message.INST_DP.REBOOT(command.getId());
		ICSDRebootCommand icsdRebootCommand = null;
		ICSDShutdownCommand icsdShutdownCommand = null;
		MoptopREBOOTQuitThread quitThread = null;
		String cLayerHostname = null;
		int cLayerPortNumber;
		boolean enable = false;

		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		try
		{
			// is reboot enabled at this level
			enable = status.getPropertyBoolean(ENABLE_PROPERTY_KEY_ROOT+
							   REBOOT_LEVEL_LIST[rebootCommand.getLevel()]);
			// if not enabled return OK
			if(enable == false)
			{
				moptop.log(Logging.VERBOSITY_VERY_TERSE,"Command:"+
					   rebootCommand.getClass().getName()+":Level:"+rebootCommand.getLevel()+
					   " is not enabled.");
				rebootDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
				rebootDone.setErrorString("");
				rebootDone.setSuccessful(true);
				return rebootDone;
			}
			// get C layer comms configuration
			getCLayerConfig();
			// do relevent reboot based on level
			switch(rebootCommand.getLevel())
			{
				case REBOOT.LEVEL_REDATUM:
					moptop.reInit();
					break;
				case REBOOT.LEVEL_SOFTWARE:
					// send software restart onto c layer.
					// send shutdown command to all C Layers
					for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
					{
						cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
						cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).
							intValue();
						sendShutdownCommand(cLayerHostname,cLayerPortNumber);
					}
					moptop.close(serverConnectionThread);
					quitThread = new MoptopREBOOTQuitThread("quit:"+rebootCommand.getId());
					quitThread.setMoptop(moptop);
					quitThread.setWaitThread(serverConnectionThread);
					// software will quit with exit value 0 as normal,
					// This will cause the autobooter to restart it.
					quitThread.start();
					break;
				case REBOOT.LEVEL_HARDWARE:
					moptop.close(serverConnectionThread);
					quitThread = new MoptopREBOOTQuitThread("quit:"+rebootCommand.getId());
					quitThread.setMoptop(moptop);
					quitThread.setWaitThread(serverConnectionThread);
					// tell the autobooter not to restart the control system
					quitThread.setExitValue(127);
					quitThread.start();
				// send reboot to the icsd_inet
					for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
					{
						cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
						moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+
						    ":processCommand:Sending reboot command to icsd_inet on machine "+
							   cLayerHostname+".");
						icsdRebootCommand = new ICSDRebootCommand(cLayerHostname);
						icsdRebootCommand.send();
					}
					break;
				case REBOOT.LEVEL_POWER_OFF:
					moptop.close(serverConnectionThread);
					quitThread = new MoptopREBOOTQuitThread("quit:"+rebootCommand.getId());
					quitThread.setMoptop(moptop);
					quitThread.setWaitThread(serverConnectionThread);
					// tell the autobooter not to restart the control system
					quitThread.setExitValue(127);
					quitThread.start();
					// We cannot implement this level at the moment
					// There is no way for the C layer to warm up the CCD, before
					// the computer is shutdown
					// So implementing this could damage the CCD, if it is powered
					// via the control computer?
				// send shutdown to the icsd_inet
					//icsdShutdownCommand = new ICSDShutdownCommand();
					//icsdShutdownCommand.send();
					break;
				default:
					moptop.error(this.getClass().getName()+
						":processCommand:"+command+":Illegal level:"+rebootCommand.getLevel());
					rebootDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1400);
					rebootDone.setErrorString("Illegal level:"+rebootCommand.getLevel());
					rebootDone.setSuccessful(false);
					return rebootDone;
			};// end switch
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+
					":processCommand:"+command+":",e);
			rebootDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1404);
			rebootDone.setErrorString(e.toString());
			rebootDone.setSuccessful(false);
			return rebootDone;
		}
		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Finished.");
	// return done object.
		rebootDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		rebootDone.setErrorString("");
		rebootDone.setSuccessful(true);
		return rebootDone;
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
	 * Send a "shutdown" command to the C layer. An instance of ShutdownCommand is used to send the command
	 * to the C layer.
	 * @param hostname A string containing the hostname to send the shutdown command to.
	 * @param portNumber The port number of the C layer to send commands to.
	 * @exception Exception Thrown if an error occurs.
	 * @see ngat.moptop.command.ShutdownCommand
	 */
	protected void sendShutdownCommand(String hostname,int portNumber) throws Exception
	{
		ShutdownCommand shutdownCommand = null;
		int returnCode,exposureStatus;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendShutdownCommand("+hostname+":"+portNumber+"):started.");
		shutdownCommand = new ShutdownCommand();
		shutdownCommand.setAddress(hostname);
		shutdownCommand.setPortNumber(portNumber);
		// actually send the command to the C layer
		shutdownCommand.sendCommand();
		// check the parsed reply
		if(shutdownCommand.getParsedReplyOK() == false)
		{
			returnCode = shutdownCommand.getReturnCode();
			errorString = shutdownCommand.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,"sendShutdownCommand:shutdown command for "+
				   hostname+":"+portNumber+" failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+":sendShutdownCommand:shutdown command for "+
					    hostname+":"+portNumber+" failed with return code "+
					    returnCode+" and error string:"+errorString);
		}
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,
			   "sendShutdownCommand("+hostname+":"+portNumber+"):finished.");
	}
}
