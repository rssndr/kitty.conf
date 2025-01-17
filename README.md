# kitty.conf

Personal kitty terminal configuration featuring:
- Color palette inspired by GNOME terminal
- Unique startup animation: a rotating, color-shifting ASCII torus

## Setup

1. Clone the repository:
```bash
git clone https://github.com/a-rossetti/kitty.conf ~/.config/kitty
cd ~/.config/kitty
```

2. Compile the welcome animation:
```bash
gcc welcome.c -o welcome -lm
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

