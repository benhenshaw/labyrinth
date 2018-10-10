# macOS
clang main.c -o labyrinth -framework SDL2 #-O3 -march=native
if [[ $? -eq 0 ]]
then
    ./labyrinth
fi
