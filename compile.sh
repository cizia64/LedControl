cp -f main.c minuisettings.txt main.ttf ../../trimui-smart-pro-toolchain/workspace/
docker exec -it trimui  gcc -o main main.c -lSDL2 -lSDL2_ttf -lm


mv -f ../../trimui-smart-pro-toolchain/workspace/main ./build/
cp -f main.ttf ./build/
cp -f minuisettings.txt ./build/