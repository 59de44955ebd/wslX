# Compiling XWin.exe

You need a recent version of Cygwin 64-bit. Open a CMD/PowerShell window, cd to your Cygwin's root folder (e.g. C:\cygwin64) and execute the following commands:

1. Install required prerequisites:
    ```
    $ setup-x86_64.exe -P binutils,bison,cygport,flex,gcc-core,git,meson,ninja,pkg-config,windowsdriproto,xorgproto,libfontenc-devel,libfreetype-devel,libGL-devel,libnettle-devel,libpixman1-devel,libtirpc-devel,libX11-devel,libXRes-devel,libXau-devel,libXaw-devel,libXdmcp-devel,libXext-devel,libXfont2-devel,libXi-devel,libXinerama-devel,libXmu-devel,libXpm-devel,libXrender-devel,libXtst-devel,libxcb-aux-devel,libxcb-composite-devel,libxcb-ewmh-devel,libxcb-icccm-devel,libxcb-image-devel,libxcb-keysyms-devel,libxcb-randr-devel,libxcb-render-devel,libxcb-render-util-devel,libxcb-shape-devel,libxcb-util-devel,libxcb-xfixes-devel,libxcb-xkb-devel,libxcvt-devel,libxkbfile-devel,font-util,ImageMagick,khronos-opengl-registry,python3-lxml,xkbcomp-devel,xtrans,cmake
    ```

2. Install xorg-server=21.1.12-1 *including its sources* (note that this command will only work if xorg-server is *not* installed yet, otherwise check the "src" checkbox in the setup's UI manually):
    ```
    $ setup-x86_64.exe -IP xorg-server=21.1.12-1
    ```
3. Switch to an interactive Cygwin64 shell
    ```
    $ bin\bash --login
    ```
4. Prepare and compile
    ```
    $ cd /usr/src/xorg-server-21.1.12-1.src
    $ cygport xorg-server.cygport prep
    $ cygport xorg-server.cygport compile
    ```
5. Replace folder 
    ```
    /usr/src/xorg-server-21.1.12-1.src/xorg-server-21.1.12-1.x86_64/src/xserver-xserver-cygwin-21-1-12-1/hw/xwin
    ```
    with the `src/xwin` folder of this repository.

6. Replace file 
    ```
    usr/src/xorg-server-21.1.12-1.src/xorg-server-21.1.12-1.x86_64/src/xserver-xserver-cygwin-21-1-12-1/meson_options.txt
    ```
    with file `src/meson_options.txt` of this repository.

7. Clean target
    ```
    $ rm -R /usr/src/xorg-server-21.1.12-1.src/xorg-server-21.1.12-1.x86_64/src/xserver-xserver-cygwin-21-1-12-1/x86_64-pc-cygwin/hw/xwin
    ```

8. Compile and strip XWin.exe
    ```
    $ cd /usr/src/xorg-server-21.1.12-1.src/xorg-server-21.1.12-1.x86_64/src/xserver-xserver-cygwin-21-1-12-1
    $ ninja -C x86_64-pc-cygwin
    $ strip x86_64-pc-cygwin/hw/xwin/XWin.exe
    ```
