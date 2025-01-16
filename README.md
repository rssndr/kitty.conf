# kitty.conf

Personal kitty terminal config based on GNOME terminal palette colors

## Setup

1. Clone the repository:
```bash
git clone https://github.com/a-rossetti/kitty.conf ~/.config/kitty
```

2. Compile the welcome animation:
```bash
cd ~/.config/kitty
gcc welcome.c -o welcome -lm
chmod +x welcome
```

3. Add this to your `~/.bashrc`:
```bash
if [[ $TERM == "xterm-kitty" && -n $PS1 ]]; then
    if [[ -z $KITTY_WELCOME_SHOWN ]]; then
        export KITTY_WELCOME_SHOWN=1
        ~/.config/kitty/welcome
    fi
fi
```

4. Reload your shell:
```bash
source ~/.bashrc
```

