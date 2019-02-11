C:\devkitPro\devkitPPC\bin\powerpc-gekko-gcc.exe -nostartfiles -nodefaultlibs -Wl,-Ttext,0x80001800 -o codehandleronly.elf codehandleronly.s
C:\devkitPro\devkitPPC\bin\powerpc-gekko-strip.exe --strip-debug --strip-all --discard-all -o codehandlersonly.elf -F elf32-powerpc codehandleronly.elf
C:\devkitPro\devkitPPC\bin\powerpc-gekko-objcopy.exe -I elf32-powerpc -O binary codehandlersonly.elf codehandleronly.bin
raw2c codehandleronly.bin
del *.elf
pause 