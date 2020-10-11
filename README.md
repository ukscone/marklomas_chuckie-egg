# marklomas_chuckie-egg
Mark Lomas's Chuckie Eggv1.1 tidied up and tweaked for Linux.

confirmed builds & runs on any x86 or ARM (Raspberry Pi) debian 
based linux, inside chromebook crostini container and probably ok 
on others too. 

removed the code:blocks/windows stuff, tidied up the build script
and moved a few things around.

uses SDL 1.2

Not sure why you'd want to but now with included Dockerfile

command to run the docker (with X11&Sound)
docker run -it --device /dev/snd --env="DISPLAY" --env="QT_X11_NO_MITSHM=1" --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" <container name>

