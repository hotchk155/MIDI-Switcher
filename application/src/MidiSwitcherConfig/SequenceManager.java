package MidiSwitcherConfig;

public class SequenceManager {
	
	public static final int NUM_PORTS = 8;
	public static final int MAX_STEPS = 32;
	public static final char TAG_STEP_TRIGGER = 'X';
	public static final char TAG_STEP_4 = '+';
	public static final char TAG_STEP = '-';
	public static final char TAG_ASSIGN = '=';
	
	boolean[][] steps = new boolean[NUM_PORTS][MAX_STEPS];
	int activeSteps = 32;
	int triggerNote = 36;
	int triggerChannel = 0;
	int playRate = 120;
	int playRepeats = 1;
	
	///////////////////////////////////////////////////////////////// 
	// Return display string for the sequence
	@Override
	public String toString()
	{
		int i,j;
		StringBuilder builder = new StringBuilder();
		builder.append("  ");
		for(j=1; j<=this.activeSteps; ++j)
		{
			if((j%4)==0)
				builder.append(j/10);
			else
				builder.append(' ');
		}
		builder.append("\r\n");
		builder.append("  ");
		for(j=1; j<=this.activeSteps; ++j)
		{
			if((j%4)==0)
				builder.append(j%10);
			else
				builder.append(' ');
		}
		builder.append("\r\n");
		for(i=0; i<NUM_PORTS; ++i)
		{
			builder.append(UserCommand.TAG_SEQ);
			builder.append((char)('a'+i));
			for(j=0; j<this.activeSteps; ++j)
			{
				if(steps[i][j])
					builder.append(TAG_STEP_TRIGGER);
				else if(((j+1)%4)==0)
					builder.append(TAG_STEP_4);
				else
					builder.append(TAG_STEP);
			}
			builder.append("\r\n");
		}		

		builder.append(UserCommand.TAG_SEQ_NOTE);
		builder.append(this.triggerNote);
		builder.append(' ');
		builder.append(UserCommand.TAG_SEQ_CHAN);
		builder.append(this.triggerChannel);
		builder.append(' ');
		builder.append(UserCommand.TAG_SEQ_LENGTH);
		builder.append(this.activeSteps);
		builder.append(' ');
		builder.append(UserCommand.TAG_SEQ_RATE);
		builder.append(this.playRate);
		builder.append(' ');
		builder.append(UserCommand.TAG_SEQ_REPEAT);
		builder.append(this.playRepeats);
		builder.append(' ');
		
		return builder.toString();
	}

	
	public boolean parseSequenceParameter(String param)
	{
		param = param.toUpperCase().trim();
		if(param.length() < 2)
			return false;
		if(param.charAt(0) < 'A' || param.charAt(0) > 'H')
			return false;
		int port = param.charAt(0) - 'A';
		int step = 0;
		for(int i=1; i<param.length(); ++i)
			steps[port][step++] = (param.charAt(i) == TAG_STEP_TRIGGER);
		while(step < MAX_STEPS)
			steps[port][step++] = false;		
		return true;
	}
		
	/////////////////////////////////////////////////////////////////
	// Execute a command
	public boolean execCommand(UserCommand command, MIDIPortManager midi)
	{
		try {
			switch(command.type)
			{
				case UserCommand.SEQ:
					if(command.parameter.length() > 0)
						return parseSequenceParameter(command.parameter);
					return true;
				case UserCommand.SEQ_NOTE:
					this.triggerNote = UserCommand.getIntParam(command.parameter, 0, 127);
					break;
				case UserCommand.SEQ_CHAN:
					this.triggerChannel = UserCommand.getIntParam(command.parameter, 1, 16);
					break;
				case UserCommand.SEQ_START:					
					break;
				case UserCommand.SEQ_RATE:
					this.playRate = UserCommand.getIntParam(command.parameter, 30, 330);
					break;
				case UserCommand.SEQ_REPEAT:
					this.playRepeats = UserCommand.getIntParam(command.parameter, 1, 10);
					break;
				case UserCommand.SEQ_LENGTH:
					this.activeSteps = UserCommand.getIntParam(command.parameter, 1, 32);
					break;
			}
			
		}
		catch(NumberFormatException e) {
			e.printStackTrace();
			return false;
		}
		/*
		catch(InvalidMidiDataException e) {
			e.printStackTrace();
			return false;
		}
		*/				
		return true;
	}
	
}
