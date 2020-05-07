// CONFIGImplementation.java
// $Id$
package ngat.moptop;

import java.lang.*;
import ngat.moptop.command.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.CONFIG;
import ngat.message.ISS_INST.CONFIG_DONE;
import ngat.message.ISS_INST.OFFSET_FOCUS;
import ngat.message.ISS_INST.OFFSET_FOCUS_DONE;
import ngat.message.ISS_INST.INST_TO_ISS_DONE;
import ngat.phase2.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the CONFIG command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 */
public class CONFIGImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Constructor. 
	 */
	public CONFIGImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.CONFIG&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.CONFIG";
	}

	/**
	 * This method gets the CONFIG command's acknowledge time.
	 * This method returns an ACK with timeToComplete set to the &quot;moptop.config.acknowledge_time &quot;
	 * held in the Moptop configuration file. 
	 * If this cannot be found/is not a valid number the default acknowledge time is used instead.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set to a time (in milliseconds).
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;
		int timeToComplete = 0;

		acknowledge = new ACK(command.getId());
		try
		{
			timeToComplete += moptop.getStatus().getPropertyInteger("moptop.config.acknowledge_time");
		}
		catch(NumberFormatException e)
		{
			moptop.error(this.getClass().getName()+":calculateAcknowledgeTime:"+e);
			timeToComplete += serverConnectionThread.getDefaultAcknowledgeTime();
		}
		acknowledge.setTimeToComplete(timeToComplete);
		return acknowledge;
	}

	/**
	 * This method implements the CONFIG command. 
	 * <ul>
	 * <li>The command is casted.
	 * <li>The DONE message is created.
	 * <li>The config is checked to ensure it isn't null and is of the right class.
	 * <li>The detector is extracted.
	 * <li>The detector is checked to ensure it is the right class.
	 * <li>The binning is checked.
	 * <li>sendConfigBinCommand is called with the binning data to send a C layer Config command.
	 * <li>sendConfigFilterCommand is called with the extracted filter data to send a C layer Config command.
	 * <li>sendConfigRotorspeedCommand is called with the rotor speed data to send a C layer Config command.
	 * <li>We test for command abort.
	 * <li>We calculate the focus offset from "moptop.focus.offset", and call setFocusOffset to tell the RCS/TCS
	 *     the focus offset required.
	 * <li>We increment the config Id.
	 * <li>We save the config name in the Moptop status instance for future reference.
	 * <li>We return success.
	 * </ul>
	 * @see #testAbort
	 * @see #setFocusOffset
	 * @see #moptop
	 * @see #status
	 * @see #sendConfigBinCommand
	 * @see #sendConfigFilterCommand
	 * @see #sendConfigRotorspeedCommand
	 * @see ngat.moptop.Moptop#getStatus
	 * @see ngat.moptop.MoptopStatus#incConfigId
	 * @see ngat.moptop.MoptopStatus#setConfigName
	 * @see ngat.phase2.MOPTOPPolarimeterConfig
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		CONFIG configCommand = null;
		MOPTOPPolarimeterDetector detector = null;
		MOPTOPPolarimeterConfig config = null;
		CONFIG_DONE configDone = null;
		String configName = null;
		float focusOffset;

		moptop.log(Logging.VERBOSITY_VERY_TERSE,"CONFIGImplementation:processCommand:Started.");
	// test contents of command.
		configCommand = (CONFIG)command;
		configDone = new CONFIG_DONE(command.getId());
		if(testAbort(configCommand,configDone) == true)
			return configDone;
		if(configCommand.getConfig() == null)
		{
			moptop.error(this.getClass().getName()+":processCommand:"+command+":Config was null.");
			configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+800);
			configDone.setErrorString(":Config was null.");
			configDone.setSuccessful(false);
			return configDone;
		}
		if((configCommand.getConfig() instanceof MOPTOPPolarimeterConfig) == false)
		{
			moptop.error(this.getClass().getName()+":processCommand:"+
				command+":Config has wrong class:"+
				configCommand.getConfig().getClass().getName());
			configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+801);
			configDone.setErrorString(":Config has wrong class:"+
				configCommand.getConfig().getClass().getName());
			configDone.setSuccessful(false);
			return configDone;
		}
	// test abort
		if(testAbort(configCommand,configDone) == true)
			return configDone;
	// get config from configCommand.
		config = (MOPTOPPolarimeterConfig)configCommand.getConfig();
	// get local detector copy
		if((config.getDetector(0) instanceof MOPTOPPolarimeterDetector) == false)
		{
			moptop.error(this.getClass().getName()+":processCommand:"+
				command+":Config detector has wrong class:"+
				config.getDetector(0).getClass().getName());
			configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+802);
			configDone.setErrorString(":Config detector has wrong class:"+
				config.getDetector(0).getClass().getName());
			configDone.setSuccessful(false);
			return configDone;
		}
		detector = (MOPTOPPolarimeterDetector)config.getDetector(0);
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"CONFIGImplementation:processCommand:Detector 0:"+detector);
		// check other  detector against this one - they must be the same
		for(int i = 1; i < config.getMaxDetectorCount(); i++)
		{
			MOPTOPPolarimeterDetector otherDetector = null;

			otherDetector = (MOPTOPPolarimeterDetector)(config.getDetector(i));
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"CONFIGImplementation:processCommand:Detector "+
				   i+":"+otherDetector);
			if((detector.getXBin() != otherDetector.getXBin())||
			   (detector.getYBin() != otherDetector.getYBin()))
			{
				moptop.error(this.getClass().getName()+":processCommand:"+
					     command+":Detector "+i+" binning ("+otherDetector.getXBin()+","+
					     otherDetector.getYBin()+") did not match first detector ("+
					     detector.getXBin()+","+detector.getYBin()+").");
				configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+807);
				configDone.setErrorString("Detector "+i+" binning  ("+otherDetector.getXBin()+","+
					     otherDetector.getYBin()+") did not match first detector ("+
					     detector.getXBin()+","+detector.getYBin()+").");
				configDone.setSuccessful(false);
				return configDone;
			}
		}
	// get configuration Id - used later
		configName = config.getId();
		moptop.log(Logging.VERBOSITY_VERY_TERSE,"Command:"+
			   configCommand.getClass().getName()+
			   "\n\t:id = "+configName+
			   "\n\t:Filter = "+config.getFilterName()+
			   "\n\t:Rotator Speed = "+config.getRotorSpeed()+
			   "\n\t:X Bin = "+detector.getXBin()+
			   "\n\t:Y Bin = "+detector.getYBin()+".");

		// send config commands to C layers
		try
		{
			sendConfigBinCommand(detector.getXBin(),detector.getYBin());
			sendConfigFilterCommand(config.getFilterName());
			sendConfigRotorspeedCommand(config.rotorSpeedToString());
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+":processCommand:"+command,e);
			configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+804);
			configDone.setErrorString(e.toString());
			configDone.setSuccessful(false);
			return configDone;
		}
	// test abort
		if(testAbort(configCommand,configDone) == true)
			return configDone;
	// Issue ISS OFFSET_FOCUS commmand. 
		try
		{
			focusOffset = status.getPropertyFloat("moptop.focus.offset");
			moptop.log(Logging.VERBOSITY_VERY_TERSE,"Command:"+
				   configCommand.getClass().getName()+":focus offset = "+focusOffset+".");
		}
		catch(NumberFormatException e)
		{
			moptop.error(this.getClass().getName()+":processCommand:"+command,e);
			configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+806);
			configDone.setErrorString(e.toString());
			configDone.setSuccessful(false);
			return configDone;
		}
		if(setFocusOffset(configCommand.getId(),focusOffset,configDone) == false)
			return configDone;
	// Increment unique config ID.
	// This is queried when saving FITS headers to get the CONFIGID value.
		try
		{
			status.incConfigId();
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+":processCommand:"+
				command+":Incrementing configuration ID:"+e.toString());
			configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+809);
			configDone.setErrorString("Incrementing configuration ID:"+e.toString());
			configDone.setSuccessful(false);
			return configDone;
		}
	// Store name of configuration used in status object
	// This is queried when saving FITS headers to get the CONFNAME value.
		status.setConfigName(configName);
	// setup return object.
		configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		configDone.setErrorString("");
		configDone.setSuccessful(true);
		moptop.log(Logging.VERBOSITY_VERY_TERSE,"CONFIGImplementation:processCommand:Finished.");
	// return done object.
		return configDone;
	}

	/**
	 * Send the extracted binning config data onto each C layer.
	 * @param xBin X/serial binning.
	 * @param yBin Y/parallel binning.
	 * @exception Exception Thrown if an error occurs.
	 * @see ngat.moptop.command.ConfigBinCommand
	 * @see ngat.moptop.command.ConfigBinCommand#setAddress
	 * @see ngat.moptop.command.ConfigBinCommand#setPortNumber
	 * @see ngat.moptop.command.ConfigBinCommand#setCommand
	 * @see ngat.moptop.command.ConfigBinCommand#sendCommand
	 * @see ngat.moptop.command.ConfigBinCommand#getParsedReplyOK
	 * @see ngat.moptop.command.ConfigBinCommand#getReturnCode
	 * @see ngat.moptop.command.ConfigBinCommand#getParsedReply
	 */
	protected void sendConfigBinCommand(int xBin,int yBin) throws Exception
	{
		ConfigBinCommand command = null;
		int portNumber,returnCode,cLayerCount;
		String hostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigBinCommand:"+
			   "\n\t:X Bin = "+xBin+
			   "\n\t:Y Bin = "+yBin+".");
		// we only support square binning
		if(xBin != yBin)
		{
			throw new Exception(this.getClass().getName()+
					    ":sendConfigBinCommand:Moptop only supports square binning: X bin = "+xBin+
					    ", yBin = "+yBin+" .");
			
		}
		cLayerCount = status.getPropertyInteger("moptop.c.count");
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			command = new ConfigBinCommand();
			// configure C comms
			hostname = status.getProperty("moptop.c.hostname."+cLayerIndex);
			portNumber = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			command.setAddress(hostname);
			command.setPortNumber(portNumber);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigBinCommand:cLayerIndex = "+cLayerIndex+
				   " :hostname = "+hostname+" :port number = "+portNumber+".");
			// set command parameters
			command.setCommand(xBin);
			// actually send the command to the C layer
			command.sendCommand();
			// check the parsed reply
			if(command.getParsedReplyOK() == false)
			{
				returnCode = command.getReturnCode();
				errorString = command.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "sendConfigBinCommand:config command failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":sendConfigBinCommand:Command failed with return code "+returnCode+
						    " and error string:"+errorString);
			}
		}// end for on cLayerIndex
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigBinCommand:finished.");
	}

	/**
	 * Send the extracted filter config data onto each C layer.
	 * @param filterName The name of the filter to be selected (put into the beam).
	 * @exception Exception Thrown if an error occurs.
	 * @see ngat.moptop.command.ConfigFilterCommand
	 * @see ngat.moptop.command.ConfigFilterCommand#setAddress
	 * @see ngat.moptop.command.ConfigFilterCommand#setPortNumber
	 * @see ngat.moptop.command.ConfigFilterCommand#setCommand
	 * @see ngat.moptop.command.ConfigFilterCommand#sendCommand
	 * @see ngat.moptop.command.ConfigFilterCommand#getParsedReplyOK
	 * @see ngat.moptop.command.ConfigFilterCommand#getReturnCode
	 * @see ngat.moptop.command.ConfigFilterCommand#getParsedReply
	 */
	protected void sendConfigFilterCommand(String filterName) throws Exception
	{
		ConfigFilterCommand command = null;
		int portNumber,returnCode,cLayerCount;
		String hostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigFilterCommand:"+
			   "\n\t:filter name = "+filterName+".");
		cLayerCount = status.getPropertyInteger("moptop.c.count");
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			command = new ConfigFilterCommand();
			// configure C comms
			hostname = status.getProperty("moptop.c.hostname."+cLayerIndex);
			portNumber = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			command.setAddress(hostname);
			command.setPortNumber(portNumber);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigFilterCommand:cLayerIndex = "+cLayerIndex+
				   " :hostname = "+hostname+" :port number = "+portNumber+".");
			// set command parameters
			command.setCommand(filterName);
			// actually send the command to the C layer
			command.sendCommand();
			// check the parsed reply
			if(command.getParsedReplyOK() == false)
			{
				returnCode = command.getReturnCode();
				errorString = command.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "sendConfigFilterCommand:config command failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":sendConfigFilterCommand:Command failed with return code "+
						    returnCode+" and error string:"+errorString);
			}
		}// end for on cLayerIndex
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigFilterCommand:finished.");
	}

	/**
	 * Send the extracted rotor speed config data onto each C layer.
	 * @param rotorSpeedString The speed of the rotator as a string.
	 * @exception Exception Thrown if an error occurs.
	 * @see ngat.moptop.command.ConfigRotorspeedCommand
	 * @see ngat.moptop.command.ConfigRotorspeedCommand#setAddress
	 * @see ngat.moptop.command.ConfigRotorspeedCommand#setPortNumber
	 * @see ngat.moptop.command.ConfigRotorspeedCommand#setCommand
	 * @see ngat.moptop.command.ConfigRotorspeedCommand#sendCommand
	 * @see ngat.moptop.command.ConfigRotorspeedCommand#getParsedReplyOK
	 * @see ngat.moptop.command.ConfigRotorspeedCommand#getReturnCode
	 * @see ngat.moptop.command.ConfigRotorspeedCommand#getParsedReply
	 */
	protected void sendConfigRotorspeedCommand(String rotorSpeedString) throws Exception
	{
		ConfigRotorspeedCommand command = null;
		int portNumber,returnCode,cLayerCount;
		String hostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigRotorspeedCommand:"+
			   "\n\t:rotor speed = "+rotorSpeedString+".");
		cLayerCount = status.getPropertyInteger("moptop.c.count");
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			command = new ConfigRotorspeedCommand();
			// configure C comms
			hostname = status.getProperty("moptop.c.hostname."+cLayerIndex);
			portNumber = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			command.setAddress(hostname);
			command.setPortNumber(portNumber);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigRotorspeedCommand:cLayerIndex = "+
				   cLayerIndex+" :hostname = "+hostname+" :port number = "+portNumber+".");
			// set command parameters
			command.setCommand(rotorSpeedString);
			// actually send the command to the C layer
			command.sendCommand();
			// check the parsed reply
			if(command.getParsedReplyOK() == false)
			{
				returnCode = command.getReturnCode();
				errorString = command.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "sendConfigRotorspeedCommand:config command failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":sendConfigRotorspeedCommand:Command failed with return code "+
						    returnCode+" and error string:"+errorString);
			}
		}// end for on cLayerIndex
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigRotorspeedCommand:finished.");
	}

	/**
	 * Routine to set the telescope focus offset, due to the filters selected. Sends a OFFSET_FOCUS command to
	 * the ISS. The OFFSET_FOCUS sent is the offset of Moptop's focus from the nominal telescope focus.
	 * @param id The Id is used as the OFFSET_FOCUS command's id.
	 * @param focusOffset The focus offset needed.
	 * @param configDone The instance of CONFIG_DONE. This is filled in with an error message if the
	 * 	OFFSET_FOCUS fails.
	 * @return The method returns true if the telescope attained the focus offset, otherwise false is
	 * 	returned an telFocusDone is filled in with an error message.
	 */
	private boolean setFocusOffset(String id,float focusOffset,CONFIG_DONE configDone)
	{
		OFFSET_FOCUS focusOffsetCommand = null;
		INST_TO_ISS_DONE instToISSDone = null;
		String filterIdName = null;
		String filterTypeString = null;

		focusOffsetCommand = new OFFSET_FOCUS(id);
	// set the commands focus offset
		focusOffsetCommand.setFocusOffset(focusOffset);
		instToISSDone = moptop.sendISSCommand(focusOffsetCommand,serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			moptop.error(this.getClass().getName()+":focusOffset failed:"+focusOffset+":"+
				     instToISSDone.getErrorString());
			configDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+805);
			configDone.setErrorString(instToISSDone.getErrorString());
			configDone.setSuccessful(false);
			return false;
		}
		return true;
	}
}
