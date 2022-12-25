# qt-messenger
Client-Server Qt messenger for kurs project

![Messenger](https://github.com/Romsik788/qt-messenger/blob/main/.github/menu.png?raw=true)
## Build
You must use Qt6
### Windows
```shell
qmake.exe "path\to\Client.pro" "CONFIG+=release"
mingw32-make.exe -f "path/to/Makefile" qmake_all
mingw32-make.exe -j4
```
### Linux
```shell
qmake "/path/to/Client.pro" "CONFIG+=release"
make -f "/path/to/Makefile" qmake_all
make -j4
```
