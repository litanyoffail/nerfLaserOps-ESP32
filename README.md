# nerfLaserOps-ESP32
Arduino code for ESP32 microcontrollers to be used as wireless targets for expanding the gameplay options of Nerf Laser Ops Classic &amp; Pro blasters in Out-Of-Box Play mode.

Flash "brain" code to one ESP32 module, and "target" code to as many other units as needed. I have not tested the upper limit on number of targets; it would theoretically be limited to the number of peers allowed for ESP-NOW.

For this iteration I used a TTGO T-Display v1.1 as the Brain module to make use of the screen, and ESP32Cam modules with USB carrier boards and IR receivers as targets. I chose my modules based on what I had on hand, not for any particular needs for the project. All units are powered by USB power banks with capacity too small to use with modern cell phones, but are fine for the microcontrollers.


Features:

-supports ALL Nerf Laser Ops Classic/Pro blasters in Out-Of-Box Play mode

-uses ESP-NOW across multiple modules using two-way communication with a "brain" unit to control "target" units

-current game modes are Random Single Target mode (cycles a single target lit, and chooses next target at random) and Patterned Multiple Target mode (chooses a pattern at random to light targets with delay)

*PATTERNED MULTIPLE ARGETS MODE is inaccessible as-is in the code; see to-do below*

-tracks hit targets, missed targets, and milliseconds to hit after lighting targets

-supports multiple blasters at once using Purple/Red/Blue codes to track scores for each player, but needs a little work to make it more interesting


To-Do:

-refine screen code for displaying scores in a more stylized way

-add functionality to cycle through game modes on button press and then "choose" the one selected when you stop pressing the button

-add additional game modes like ones requiring more than 3 targets, or co-operative target shooting
