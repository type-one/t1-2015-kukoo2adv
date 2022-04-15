What
----

Kukoo 2 Advance GBA BBS-tro source code

https://www.youtube.com/watch?v=uV3GsSpxdV0

Released 1 August 2015
4th in the Evoke 2015 Alternative Platforms competition
Nintendo GameBoy Advance (GBA)

Project started in 2001-2002 (State Of The Art 2002) but paused for a decade. 
Resumed by the end of 2014 as a "come back" to the demo-scene.

https://demozoo.org/productions/142976/


Build
-----

buildable with devkitPro and the legacy devkitPro/devkitARM/r42 (2014) to r45 (2015)

https://devkitpro.org/

https://wii.leseratte10.de/devkitPro/devkitARM/r42%20(2014)/
https://wii.leseratte10.de/devkitPro/devkitARM/r45%20(2015)/

In your devkitPro folder, unzip the r42/r45 devkitARM and rename it to devkitARM_r42/devkitARM_r45

On Windows, launch MSYS2
```
cd src
make
```

Run the GBA rom in an emulator like VisualBoyAdvance or on a real target (GBA, GBA SP, NDS)

If you want to rebuild the **krawall** librery
```
cd krawall
cd lib
cmake .
make
```

Then copy the **libkrawall.a** file to src
