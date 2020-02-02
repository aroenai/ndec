# ndec
ndec is an extremely straightforward zelda ROM decompressor written in straight C. ROM must be in big endian.

Original by Wareya:
https://archive.is/8h67Q

## Compiling on MinGW
Follow the steps here to setup your base environment:
https://github.com/orlp/dev-on-windows/wiki/Installing-GCC--&-MSYS2

```
gcc ndec.c -o ndec.exe
strip ndec.exe
```

## Compiling on Debian/Ubuntu
```
sudo apt-get install build-essential git
git clone https://github.com/aroenai/ndec.git ~/ndec
cd ~/ndec
gcc ndec.c -o ndec
strip ndec
```
