This firmware is for the Teensy 3.6. It is set up to be compiled with
    PlatformIO. As of uploading this, for some reason gcc links the wrong
    ARM math library by default, causing build errors. Teensy 3.6 uses hardware
    floating point, so GCC should be linking to libarm_cortexM4lf_math.a, but
    instead it links libarm_cortexM4l_math.a. The hacky and dumb fix to this is
    to rename libarm_cortexM4lf_math.a to libarm_cortexM4l_math.a. My libraries
    were located in:

    AppData\Roaming\SPB_16.6\.platformio\packages\toolchain-gccarmnoneeabi\arm-none-eabi\lib
