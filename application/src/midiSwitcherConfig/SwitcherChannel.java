package midiSwitcherConfig;

import javax.sound.midi.InvalidMidiDataException;

public class SwitcherChannel {
	
	// MIDI NRPN command LSB
	public static final int NRPN_LO_COMMIT = 0;
	public static final int NRPN_LO_TRIGGERCHANNEL = 1;
	public static final int NRPN_LO_TRIGGERNOTE = 2;
	public static final int NRPN_LO_DURATION = 3;
	public static final int NRPN_LO_DURATIONMODULATOR = 4;
	public static final int NRPN_LO_DUTY = 5;
	public static final int NRPN_LO_DUTYMODULATOR = 6; 
	public static final int NRPN_LO_INVERT = 7;

	// Modulators
	public static final int MODULATOR_NONE = 0x80;
	public static final int MODULATOR_NOTEVELOCITY = 0x81;
	public static final int MODULATOR_PITCHBEND = 0x82;

	// Special command tags for modulators
	public static final String TAG_VELOCITY = "v";
	public static final String TAG_PITCHBEND = "p";
	
	// Channel config
	int triggerChannel = 0;
	int triggerNote = 60;
	int duration = 5;
	int durationModulator = MODULATOR_NONE;
	int duty=100;
	int dutyModulator = MODULATOR_NONE;
	int invert=0;

	/////////////////////////////////////////////////////////////////
	// Display string suffix for a modulator
	public String formatModulator(int value)
	{
		switch(value)
		{
		case MODULATOR_NONE:
			return "";
		case MODULATOR_NOTEVELOCITY:
			return TAG_VELOCITY;
		case MODULATOR_PITCHBEND:
			return TAG_PITCHBEND;
		default:
			return Integer.toString(value);
		}
	}
	
	///////////////////////////////////////////////////////////////// 
	// Return display string for the channel
	public String format()
	{
		StringBuilder builder = new StringBuilder();
		
		// chan1
		builder.append(UserCommand.TAG_CHANNEL);
		builder.append(this.triggerChannel+1);		
		builder.append(' ');

		// @60
		builder.append(UserCommand.TAG_NOTE);
		builder.append(this.triggerNote);		
		builder.append(' ');

		// dur20
		builder.append(UserCommand.TAG_DURATION);
		builder.append(this.duration);		
		builder.append(' ');

		// dur~v
		if(this.durationModulator != MODULATOR_NONE)
		{
			builder.append(UserCommand.TAG_DURATION_MODULATOR);
			builder.append(formatModulator(this.durationModulator));		
			builder.append(' ');
		}
		
		// duty100
		builder.append(UserCommand.TAG_DUTY);
		builder.append(this.duty);		
		builder.append(' ');

		// duty~v
		if(this.dutyModulator != MODULATOR_NONE)
		{
			builder.append(UserCommand.TAG_DUTY_MODULATOR);
			builder.append(formatModulator(this.dutyModulator));		
			builder.append(' ');
		}
		
		// inv0
		builder.append(UserCommand.TAG_INVERT);
		builder.append(this.invert);		
		builder.append(' ');
		
		return builder.toString();
	}

	
	/////////////////////////////////////////////////////////////////
	// Parse a modulator parameter
	public static int getModulatorParam(String param, int min, int max)
	{
		if(param.length() == 0)
			return MODULATOR_NONE;
		if(param.equals(TAG_PITCHBEND))
			return MODULATOR_PITCHBEND;
		if(param.equals(TAG_VELOCITY))
			return MODULATOR_NOTEVELOCITY;
		return UserCommand.getIntParam(param, min, max);
		
	}
		
	/////////////////////////////////////////////////////////////////
	// Execute a command
	public boolean execCommand(UserCommand command, int sequence, MIDIPortManager midi)
	{
		try {
			int nrpnHi = sequence;
			switch(command.type)
			{
				case UserCommand.CHANNEL:
					this.triggerChannel = UserCommand.getIntParam(command.parameter,1,16)-1;
					midi.sendNRPN(nrpnHi, NRPN_LO_TRIGGERCHANNEL, this.triggerChannel);
					break;
				case UserCommand.NOTE:
					this.triggerNote = UserCommand.getIntParam(command.parameter,0,127);
					midi.sendNRPN(nrpnHi, NRPN_LO_TRIGGERNOTE, this.triggerNote);
					break;
				case UserCommand.DURATION:
					this.duration = UserCommand.getIntParam(command.parameter,0,10000);
					midi.sendNRPN(nrpnHi, NRPN_LO_DURATION, this.duration);
					break;
				case UserCommand.DURATION_MODULATOR:
					this.durationModulator = getModulatorParam(command.parameter, 1, 127);
					midi.sendNRPN(nrpnHi, NRPN_LO_DURATIONMODULATOR, this.durationModulator);
					break;
				case UserCommand.DUTY:
					this.duty = UserCommand.getIntParam(command.parameter,0,100);
					midi.sendNRPN(nrpnHi, NRPN_LO_DUTY, this.duty);
					break;
				case UserCommand.DUTY_MODULATOR:
					this.dutyModulator = getModulatorParam(command.parameter, 1, 127);
					midi.sendNRPN(nrpnHi, NRPN_LO_DUTYMODULATOR, this.dutyModulator);
					break;
				case UserCommand.INVERT:
					this.invert = UserCommand.getIntParam(command.parameter,0,1);
					midi.sendNRPN(nrpnHi, NRPN_LO_INVERT, this.invert);
					break;
				case UserCommand.TRANSMIT_ALL:
					midi.sendNRPN(nrpnHi, NRPN_LO_TRIGGERCHANNEL, this.triggerChannel);
					midi.sendNRPN(nrpnHi, NRPN_LO_TRIGGERNOTE, this.triggerNote);
					midi.sendNRPN(nrpnHi, NRPN_LO_DURATION, this.duration);
					midi.sendNRPN(nrpnHi, NRPN_LO_DURATIONMODULATOR, this.durationModulator);
					midi.sendNRPN(nrpnHi, NRPN_LO_DUTY, this.duty);
					midi.sendNRPN(nrpnHi, NRPN_LO_DUTYMODULATOR, this.dutyModulator);
					midi.sendNRPN(nrpnHi, NRPN_LO_INVERT, this.invert);
					break;
				case UserCommand.COMMIT:
					midi.sendNRPN(nrpnHi, NRPN_LO_COMMIT, 1);
					break;
				case UserCommand.TRIGGER:
					midi.sendTrigger(this.triggerChannel, this.triggerNote);
					break;
				default:
					return false;
			}
			
		}
		catch(NumberFormatException e) {
			e.printStackTrace();
			return false;
		}
		catch(InvalidMidiDataException e) {
			e.printStackTrace();
			return false;
		}				
		return true;
	}
	
}
