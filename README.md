# Morserator

A Morse code (CW) decoder

## Caveat Emptor

Morserator is written using SDL2 and has a staggeringly ugly UI.  At this point it is definitely a work-in-progress but, if you want to try it, be my guest.

When it's running you'll see the Morse scrolling, with the decoded letters superimposed on the waterfall.  Click on the 
Morse channel and it'll take you to a text box containing the decoded text.  Click again to return to the waterfall.

There's something wrong with the text box: it hangs after a while.  Sometimes the fist isn't identified correctly and all you get are Es and Ts.  But when it does work it decodes way better than I can.

Like I said: it's a work in progress.

## About

### Decode Data Flow

#### FFT channeliser

First step is to divide the input into channels.  An 8k audio input stream is resampled to 6400 then broken into blocks of 128 samples and fed into a FFT, giving a sample rate and an output channel spacing of 50Hz.

The magnitudes of these channels is shown on a "waterfall" display scrolling right-to-left.

Each channel is sampled for energy using a suitable detector.   Morse decoding doesn't care about the purity of the waveform.

An output energy value for each cell is generated for every 60 samples, so giving a sample rate of 50 samples/second.  This corresponds to a Morse rate of 60 PARIS or 50 CODEX per minute.  

#### Morse energy decoder

The first step is to search the input sample for Morse-like energy patterns.  The first stage of this is level detection.  The energy samples can be expected to cluster around two levels: an "on" level and an "off" level.  A histogram can be used to identify the average "on" energy level and the average "off" energy level.  If the histogram does not contain a double peak, the sample is assumed not to contain Morse code.

Once the "on" and "off" levels have been identified, two new histograms can be created.  These are for the durations of the "on" and "off" periods in the sample.  The "on" durations can be expected to show peaks at the dit and dah durations; and the off durations can be expected to show peaks at the dit, letter-space and word-space durations.  These values can be used to populate the fist structure, to assist with decoding.

A possible improvement is to use the fist values to low-pass filter the energy sample, to eliminate high frequency noise.

Assuming that the durations resulted in an identifiable fist, the decoding can begin.  Each possible symbol is generated with the scanned fist, and compared to the received energy pattern.  The closest symbol is selected and recorded, along with a snr value.  Then, based on this timing, the next symbol is searched for, until the whole sample is decoded.



## Implementation


A game library is used for quick graphics, soundcard access, asynchronous operation and portability.


## Building


To build in a Linux-type environment, the usual build tools (C, Make) are needed, along with the SDL2 development libraries.  For Debian and friends, that's something like "sudo apt install sdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev"

If you have SDL2 installed and a C compiler, "make" should work.


## Using

### Linux

To configure your devices you'll have to use the ~/.morserator configuration file.  Something like this worked for me:-

	version: 0.0
	audio_in: USB Audio Device Mono
	audio_out: USB Audio Device Mono

Edit yours to work in your system and with your devices.  


## Appendix: WPM Standards

	PARIS: .--. .- .-. .. ...    10xDit(+1) 4xDah(+3) 9xSpace(-1) 4xLspace(-3) 1xWspace(-7) -> 50 
	CODEX: -.-. --- -.. . -..-    7xDit(+1) 8xDah(+3) 10xSpace(-1) 4xLspace(-3) 1xWspace(-7) -> 60

