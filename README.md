## ATtiny Spectrum Analyzer

This is an 8-channel spectrum analyzer using ATtiny84A and an electret microphone.  The 20x gain stage of the differential ADC is used so a separate amplifier is not necessary.

The code uses a DFT algorithm by Łukasz Podkalicki.  Original code was based on a [blog post](https://blog.podkalicki.com/attiny13-disco-lights-using-dft/), but after I contacted him he kindly shared a newer version of the code which I don't believe he had published previously.  I adapted the newest code to the ATtiny84A with 8 channels and tweaked the thresholds to work with an electret microphone.

The code for an earlier experiment with the ATtiny13A is also included. The 13A doesn't include a gain stage on it's ADC so a LM386 audio amplifier was used to amplify the audio signal before passing it into the 13's ADC.

I have also designed a PCB for the ATtiny84A version but I haven't had it manufactured yet.