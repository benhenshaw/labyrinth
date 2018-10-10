# macOS / clang
clang main.c -o labyrinth -framework SDL2
if [[ $? -eq 0 ]]
then
    ./labyrinth
fi
