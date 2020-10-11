# marklomas_chuckie-egg
Mark Lomas's Chuckie Eggv1.1 tidied up and tweaked for Raspberry Pi/Linux

removed the code:blocks/windows stuff and tidied up the build script.
uses SDL 1.2

Not sure why you'd want to but now with included Dockerfile

command to run the docker (with X11&Sound)
docker run -it --device /dev/snd --env="DISPLAY" --env="QT_X11_NO_MITSHM=1" --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" <container name>

