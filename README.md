This is a synthesizer project. The goal is to build something like a modular
synthesizer.

Code is in need of a rewrite and an actually usuable user interface
that doesnt require you to memorize the commands to get to different screens,
but currently supports these features:

Additive Synthesis
Subtractive Synthesis
FM Synthesis
Scanned Synthesis
Reverb Unit


I want to give thanks to Adventure Kid
https://www.adventurekid.se/akrt/waveforms/
for compiling this cool set of waveforms that
I use with the scanned synthesis tool.

Good Luck getting this to run on your computer.
I hardcoded screen dimensions and theres no telling
what else will cause problems.

Oh I also hardcoded for this midi controller:
https://www.amazon.com/gp/product/B01DKQZFSC/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1

On Ubuntu you will need to:
apt install libx11-dev

Compile with
make

Run with
./sy <midi keyboard> <midi controller>

Ex: 
./sy /dev/midi3 /dev/midi2

Press 'r' to enter the reverb screen and use the controller to change the values.
Press 'm' to enter the scanned synthesis screen and use the controller to change the values.

