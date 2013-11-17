package MidiSwitcherConfig;

import java.util.Vector;

import javax.sound.midi.InvalidMidiDataException;
import javax.sound.midi.MidiDevice;
import javax.sound.midi.MidiSystem;
import javax.sound.midi.MidiUnavailableException;
import javax.sound.midi.Receiver;
import javax.sound.midi.ShortMessage;

public class MIDIPortManager {

	public static final byte CC_NRPN_HI = 99;
	public static final byte CC_NRPN_LO = 98;
	public static final byte CC_DATA_ENTRY_HI  = 6;
	public static final byte CC_DATA_ENTRY_LO = 38;
	
	Vector<MidiDevice.Info> midiPortInfo;
	MidiDevice.Info activeDeviceInfo;
	MidiDevice activeDevice;
	Receiver activeReceiver;
	public MIDIPortManager()
	{
		midiPortInfo = new Vector<MidiDevice.Info>(); 
		rescan();
	}
	void pause(int ms)
	{
		try {
			Thread.sleep(ms);
		} catch (InterruptedException e) {}
	}
	void rescan()
	{
		midiPortInfo.clear();
		
		// Obtain information about all the installed synthesizers.		
		MidiDevice.Info[] infos = MidiSystem.getMidiDeviceInfo();
		for (int i = 0; i < infos.length; i++) {
		    try {
		    	MidiDevice device = MidiSystem.getMidiDevice(infos[i]);
		    	if (device.getMaxReceivers()!=0)
		    	{
			    	midiPortInfo.add(infos[i]);
			    }
		    } catch (MidiUnavailableException e) {
		          // Handle or throw exception...
		    }
		}
	}
	@Override
	public String toString()
	{		
		StringBuilder builder = new StringBuilder();
		builder.append("Installed MIDI output devices:\r\n");
		int seq = 1;
		for(MidiDevice.Info dev : midiPortInfo)
		{
			if(activeDeviceInfo != null &&
				activeDeviceInfo.getName().equals(dev.getName()) &&
				activeDeviceInfo.getVendor().equals(dev.getVendor()) &&
				activeDeviceInfo.getVersion().equals(dev.getVersion()))
			{
				builder.append(">");
			}
			else
			{
				builder.append(" ");
			}
				
			builder.append(Integer.toString(seq));
			builder.append(". ");
			builder.append(dev.getName());
			builder.append("\r\n");
			seq++;
		}
		return builder.toString();
	}
	public boolean execCommand(UserCommand command)
	{		
		try {
			switch(command.type)
			{
				case UserCommand.DEVICE:
					if(command.parameter.length() == 0)
					{
						rescan();
						return true;
					}
					else
					{				
						int dev = UserCommand.getIntParam(command.parameter, 0,Integer.MAX_VALUE);
						if(dev < 1 || dev > midiPortInfo.size())
							return false;
						activeDeviceInfo = midiPortInfo.get(dev-1);
						try {
							MidiDevice device = MidiSystem.getMidiDevice(activeDeviceInfo);
							if(device != activeDevice)
							{
								if(activeReceiver != null)
									activeReceiver.close();
								activeReceiver = null;
								
								if(activeDevice != null )
									activeDevice.close();
								activeDevice = device;								
								activeDevice.open();
								
								activeReceiver = activeDevice.getReceiver();
							}
						}
						catch(MidiUnavailableException e)
						{
							e.printStackTrace();
							activeDevice = null;
							activeReceiver = null;
							return false;
						}
						return (activeReceiver != null);
					}
				default:
					return false;
			}
		}
		catch(NumberFormatException e) {
			return false;
		}
	}
	
	
	void sendNRPN(int nrpnHi, int nrpnLo, int data) throws InvalidMidiDataException
	{
		if(activeReceiver != null)
		{
			ShortMessage msg;
			
			msg = new ShortMessage(); 			
			msg.setMessage(ShortMessage.CONTROL_CHANGE, 0, CC_NRPN_HI, nrpnHi);
			activeReceiver.send(msg, -1);
			pause(2);
			msg = new ShortMessage(); 			
			msg.setMessage(ShortMessage.CONTROL_CHANGE, 0, CC_NRPN_LO, nrpnLo);
			activeReceiver.send(msg, -1);
			pause(2);			
			msg = new ShortMessage(); 			
			msg.setMessage(ShortMessage.CONTROL_CHANGE, 0, CC_DATA_ENTRY_HI, (data>>7) & 0x7f);
			activeReceiver.send(msg, -1);
			pause(2);
			msg = new ShortMessage(); 			
			msg.setMessage(ShortMessage.CONTROL_CHANGE, 0, CC_DATA_ENTRY_LO, data & 0x7f);
			activeReceiver.send(msg, -1);
			pause(2);			
		} 
	}
	
	void sendTrigger(int channel, int note) throws InvalidMidiDataException
	{
		if(activeReceiver != null)
		{
			ShortMessage msg;
			
			msg = new ShortMessage(); 			
			msg.setMessage(ShortMessage.NOTE_ON, channel, note, 127);
			activeReceiver.send(msg, -1);
			pause(2);			
			msg = new ShortMessage(); 			
			msg.setMessage(ShortMessage.NOTE_ON, channel, note, 0);
			activeReceiver.send(msg, -1);			
			pause(2);			
		}
	}
}
