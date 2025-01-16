#!/bin/bash
clear

echo " $(tput setaf 2) Welcome!$(tput sgr0)"
echo ""
echo " $(tput setaf 2)$(date '+%A, %B %d, %Y | %H:%M')$(tput sgr0)"
echo ""

~/.config/kitty/torus & TORUS_PID=$!

read -n 1 -s -r -p ""
kill $TORUS_PID 2>/dev/null
clear
exec $SHELL

