#!/bin/sh

### ################################################################################################################################

### ################################
### FreeBSD Handbook
### ################################

### https://docs.freebsd.org/en/books/handbook/

### ################################
### FreeBSD Developers' Handbook
### ################################

### https://docs.freebsd.org/en/books/developers-handbook/

### ################################
### FreeBSD FAQs
### ################################

### https://docs.freebsd.org/en/books/faq/

### ################################################################################################################################

### ################################
### Setup System
### ################################

# GROUPS
pw groupmod wheel -m gabriel
pw groupmod video -m gabriel

# PACKAGE
pkg bootstrap --yes
pkg update
pkg upgrade --yes

# VMWARE
pkg install --yes open-vm-tools
pkg install --yes xf86-video-vmware
pkg install --yes xf86-input-vmmouse
sysrc vmware_guest_kmod_enable="YES"
sysrc vmware_guestd_enable="YES"

# KVM/QEMU
pkg install --yes xf86-video-qxl
pkg install --yes qemu-guest-agent
sysrc qemu_guest_agent_enable="YES"
sysrc spice_vdagentd_enable="YES"

# UEFI
pkg install --yes xf86-video-scfb

# TERMINAL
sysrc allscreens_flags="-f spleen-16x32"

# SUDO
pkg install --yes sudo
cat << 'EOF' | tee "/usr/local/etc/sudoers.d/wheel" > "/dev/null"
%wheel ALL=(ALL:ALL) NOPASSWD: ALL
EOF
chmod 0440 "/usr/local/etc/sudoers.d/wheel"

# DOAS
pkg install --yes doas
cat << 'EOF' | tee "/usr/local/etc/doas.conf" > "/dev/null"
permit nopass :wheel
EOF
chmod 0440 "/usr/local/etc/doas.conf"

### ################################
### Setup Environment
### ################################

# DESKTOP
sudo pkg install --yes desktop-installer
sudo desktop-installer

# GNOME X11
sudo mv /usr/local/share/xsessions/gnome-classic.desktop /usr/local/share/xsessions/gnome-classic.desktop.bak
sudo mv /usr/local/share/xsessions/gnome-classic-xorg.desktop /usr/local/share/xsessions/gnome-classic-xorg.desktop.bak

# GNOME WAYLAND
sudo mv /usr/local/share/wayland-sessions/gnome-classic.desktop /usr/local/share/wayland-sessions/gnome-classic.desktop.bak
sudo mv /usr/local/share/wayland-sessions/gnome-classic-wayland.desktop /usr/local/share/wayland-sessions/gnome-classic-wayland.desktop.bak

### ################################
### System Environment
### ################################

# SHORTCUT
gsettings set "org.gnome.desktop.wm.keybindings"                "show-desktop"  "[]"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "home"          "['<Control><Alt>h']"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "calculator"    "['<Control><Alt>c']"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "www"           "['<Control><Alt>g']"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "search"        "['<Control><Alt>f']"

### ################################
### Custom Settings
### ################################

# CREATE LAUNCHER FUNCTION
create_launcher() {
	local INDEX="$1"
	local NAME="$2"
	local COMMAND="$3"
	local BINDING="$4"
	local KEY_PATH="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom${INDEX}/"

	gsettings set "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$KEY_PATH" "name" "$NAME"
	gsettings set "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$KEY_PATH" "command" "$COMMAND"
	gsettings set "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$KEY_PATH" "binding" "$BINDING"

	local CURRENT_LIST
	CURRENT_LIST=$(gsettings get "org.gnome.settings-daemon.plugins.media-keys" custom-keybindings)

	if [[ "$CURRENT_LIST" != *"$KEY_PATH"* ]]; then
		local NEW_LIST
		if [ "$CURRENT_LIST" = "@as []" ]; then
			NEW_LIST="['$KEY_PATH']"
		else
			NEW_LIST="${CURRENT_LIST%]}, '$KEY_PATH']"
		fi
		gsettings set "org.gnome.settings-daemon.plugins.media-keys" custom-keybindings "$NEW_LIST"
		echo "✅ $NAME added successfully."
	else
		echo "ℹ️  $NAME was already configured."
	fi
}

# CREATE LAUNCHERS
create_launcher 0   "Launch Settings"       "gnome-control-center"  "<Control><Alt>s"
create_launcher 1   "Launch Console"        "kgx"                   "<Control><Alt>t"
create_launcher 2   "Launch Terminal"       "ptyxis"                "<Control><Alt>y"
create_launcher 3   "Launch Emacs"          "emacs"                 "<Control><Alt>e"

### ################################################################################################################################

### ################################
### Setup Wget
### ################################

# WGET
sudo pkg install --yes wget
sudo pkg install --yes wget2
sudo pkg install --yes curl

### ################################
### Setup Git
### ################################

# GIT
sudo pkg install --yes git
sudo pkg install --yes git-credential-oauth

# SETUP
rm "${HOME}/.gitconfig"
git config --global credential.helper "!gh auth git-credential"
git config --global user.email "gabriel.frigo4@gmail.com"
git config --global user.name "Gabriel Frigo"
git config --global init.defaultBranch "main"
git config --global pull.rebase false
git config --global color.ui auto

# GITHUB
sudo pkg install --yes gh
gh auth login
gh auth setup-git

### ################################
### Setup Ports
### ################################

# PORTS
sudo git clone "https://git.FreeBSD.org/ports.git" "/usr/ports"

# UPDATE
cd "/usr/ports"
sudo git pull
cd ~

### ################################################################################################################################

### ################################
### Setup Shell
### ################################

# Default Shell
sudo chsh -s "$(which sh)" "$(whoami)"
sudo chsh -s "$(which sh)" "root"

# Kgx Config
SHELL_BLOCK="$(mktemp)"
cat << 'EOF' > "${SHELL_BLOCK}"
### ################################
### TERMINAL ENVIRONMENT
### ################################

if [ -z "${SHELL_INIT}" ]; then
	if [ -z "${KGX_SHELL}" ]; then
		KGX_SHELL="$(which zsh)"
	fi

	if [ -x "${KGX_SHELL}" ]; then
		export SHELL="${KGX_SHELL}"
		unset SHELL_INIT KGX_SHELL
		printf "\033[H\033[2J\033[3J"
		exec "${SHELL}"
	else
		unset SHELL_INIT KGX_SHELL
	fi
fi

### ################################
### BASIC ENVIRONMENT
### ################################
EOF
touch "${HOME}/.shrc"
cat "${SHELL_BLOCK}" "${HOME}/.shrc" > "${HOME}/.shrc.tmp" && mv "${HOME}/.shrc.tmp" "${HOME}/.shrc"
sudo touch "/root/.shrc"
sudo cat "${SHELL_BLOCK}" "/root/.shrc" | sudo tee "/root/.shrc.tmp" > /dev/null && sudo mv "/root/.shrc.tmp" "/root/.shrc"
rm "${SHELL_BLOCK}"

# Config Shell
cat << 'EOF' | tee -a "${HOME}/.shrc" | sudo tee -a "/root/.shrc" > "/dev/null"
### ################################
### SHELL ENVIRONMENT
### ################################

export SHELL_INIT=1

### ################################
### SHELL APPEARANCE
### ################################

C_RESET="\[\e[0m\]"

C_NORM_BLACK="\[\e[0;30m\]"
C_NORM_RED="\[\e[0;31m\]"
C_NORM_GREEN="\[\e[0;32m\]"
C_NORM_YELLOW="\[\e[0;33m\]"
C_NORM_BLUE="\[\e[0;34m\]"
C_NORM_MAGENTA="\[\e[0;35m\]"
C_NORM_CYAN="\[\e[0;36m\]"
C_NORM_WHITE="\[\e[0;37m\]"

C_BRT_GRAY="\[\e[1;90m\]"
C_BRT_RED="\[\e[1;91m\]"
C_BRT_GREEN="\[\e[1;92m\]"
C_BRT_YELLOW="\[\e[1;93m\]"
C_BRT_BLUE="\[\e[1;94m\]"
C_BRT_MAGENTA="\[\e[1;95m\]"
C_BRT_CYAN="\[\e[1;96m\]"
C_BRT_WHITE="\[\e[1;97m\]"

update_prompt() {
	local branch="$(command git symbolic-ref --short HEAD 2>/dev/null || command git rev-parse --short HEAD 2>/dev/null)"
	local git_info=" "

	if [ -n "${branch}" ]; then
		git_info=" ${C_BRT_BLUE}(${C_BRT_RED}${branch}${C_BRT_BLUE})${C_RESET} "
	fi

	local usr_color
	if [ "$(id -u)" -eq 0 ]; then
		usr_color="${C_BRT_RED}"
	else
		usr_color="${C_BRT_GREEN}"
	fi

	local cur_user="${USER:-$(id -un)}"
	local cur_host="$(hostname -s)"

	local cur_dir
	if [ "${PWD}" = "${HOME}" ]; then
		cur_dir="~"
	elif [ "${PWD}" = "/" ]; then
		cur_dir="/"
	else
		cur_dir="$(basename "${PWD}")"
	fi

	export PS1="${usr_color}${cur_user}${C_BRT_BLUE}@${C_BRT_MAGENTA}${cur_host}${C_BRT_GRAY}:${C_BRT_GRAY}[${C_BRT_YELLOW}${cur_dir}${C_BRT_GRAY}]${C_RESET}${git_info}${C_BRT_CYAN}\$${C_RESET} "
}
update_prompt

run_and_update() {
	local cmd="$1"
	shift
	command "$cmd" "$@"
	local ret=$?
	update_prompt
	return $ret
}

TRIGGER_COMMANDS="cd rm rmdir git gh wget curl unzip tar 7z"
for cmd in $TRIGGER_COMMANDS; do
	eval "${cmd}() { run_and_update ${cmd} \"\$@\"; }"
done

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

# Commands ALIAS
alias clear='printf "\033[H\033[2J\033[3J"'
# Packages ALIAS
alias uppkg='sudo pkg update && sudo pkg upgrade --yes'
alias upall='uppkg'

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################
### Setup Bash
### ################################

# Install Bash
sudo pkg install --yes bash

# Config Bash
cat << 'EOF' | tee -a "${HOME}/.bashrc" | sudo tee -a "/root/.bashrc" > "/dev/null"
### ################################
### SHELL ENVIRONMENT
### ################################

export SHELL_INIT=1

### ################################
### SHELL APPEARANCE
### ################################

C_RESET="\[\e[0m\]"

C_NORM_BLACK="\[\e[0;30m\]"
C_NORM_RED="\[\e[0;31m\]"
C_NORM_GREEN="\[\e[0;32m\]"
C_NORM_YELLOW="\[\e[0;33m\]"
C_NORM_BLUE="\[\e[0;34m\]"
C_NORM_MAGENTA="\[\e[0;35m\]"
C_NORM_CYAN="\[\e[0;36m\]"
C_NORM_WHITE="\[\e[0;37m\]"

C_BRT_GRAY="\[\e[1;90m\]"
C_BRT_RED="\[\e[1;91m\]"
C_BRT_GREEN="\[\e[1;92m\]"
C_BRT_YELLOW="\[\e[1;93m\]"
C_BRT_BLUE="\[\e[1;94m\]"
C_BRT_MAGENTA="\[\e[1;95m\]"
C_BRT_CYAN="\[\e[1;96m\]"
C_BRT_WHITE="\[\e[1;97m\]"

git_branch() {
	local branch="$(git symbolic-ref --short HEAD 2>/dev/null || git rev-parse --short HEAD 2>/dev/null)"
	if [ -n "${branch}" ]; then
		echo "${branch}"
	fi
}

show_git_branch() {
	if git rev-parse --is-inside-work-tree &>/dev/null; then
		local branch="$(git_branch)"
		if [ -n "${branch}" ]; then
			echo "❮${C_BRT_RED}󰊢 ${C_BRT_MAGENTA}${branch}${C_NORM_YELLOW}❯"
		fi
	fi
}

os_version=$(freebsd-version)
sh_name=$(ps -p $$ -o comm=)
if [ "$(id -u)" -eq 0 ]; then
	usr_color="${C_BRT_RED}"
else
	usr_color="${C_BRT_GREEN}"
fi

update_prompt() {
	PS1="\n${C_NORM_YELLOW}${C_BRT_RED} ${C_BRT_MAGENTA}${os_version}${C_NORM_YELLOW}─${C_BRT_BLUE} ${C_BRT_MAGENTA}${sh_name}${C_NORM_YELLOW}"
	PS1+="\n${C_NORM_YELLOW}┌──❮ ${C_BRT_GREEN} \t${C_NORM_YELLOW} ❯─❮ ${C_BRT_GREEN} \D{%d/%m/%y}${C_NORM_YELLOW} ❯─❮ ${C_BRT_YELLOW} ${C_BRT_CYAN}\W${C_NORM_YELLOW} ❯─ ❮${C_BRT_BLUE} ${usr_color}\u${C_NORM_YELLOW}❯ $(show_git_branch)"
	PS1+="\n${C_NORM_YELLOW}└─${C_BRT_BLUE}${C_RESET} "
}
PROMPT_COMMAND=update_prompt

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

# Commands ALIAS
alias clear='printf "\e[H\e[2J\e[3J"'
# Packages ALIAS
alias uppkg='sudo pkg update && sudo pkg upgrade --yes'
alias upall='uppkg'

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################
### Setup Zsh
### ################################

# Install Zsh
sudo pkg install --yes zsh
curl -fsSL "https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh" | sh -s -- --unattended
curl -fsSL "https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh" | sudo sh -s -- --unattended

# Config Zsh
cat << 'EOF' | tee -a "${HOME}/.zshrc" | sudo tee -a "/root/.zshrc" > "/dev/null"
### ################################
### SHELL OPTIONS SETUP
### ################################

# History OPTIONS
setopt SHARE_HISTORY
setopt HIST_IGNORE_DUPS
setopt HIST_IGNORE_SPACE
setopt HIST_REDUCE_BLANKS

# Globbing & Expansion OPTIONS
setopt EXTENDED_GLOB
setopt GLOB_DOTS
setopt PROMPT_SUBST

# Interaction OPTIONS
setopt CORRECT
setopt INTERACTIVE_COMMENTS
unsetopt BEEP

# Navigation OPTIONS
setopt AUTO_CD

### ################################
### SHELL ENVIRONMENT
### ################################

export SHELL_INIT=1

### ################################
### SHELL APPEARANCE
### ################################

local C_RESET="%f%b"

local C_NORM_BLACK="%b%F{0}"
local C_NORM_RED="%b%F{1}"
local C_NORM_GREEN="%b%F{2}"
local C_NORM_YELLOW="%b%F{3}"
local C_NORM_BLUE="%b%F{4}"
local C_NORM_MAGENTA="%b%F{5}"
local C_NORM_CYAN="%b%F{6}"
local C_NORM_WHITE="%b%F{7}"

local C_BRT_GRAY="%B%F{8}"
local C_BRT_RED="%B%F{9}"
local C_BRT_GREEN="%B%F{10}"
local C_BRT_YELLOW="%B%F{11}"
local C_BRT_BLUE="%B%F{12}"
local C_BRT_MAGENTA="%B%F{13}"
local C_BRT_CYAN="%B%F{14}"
local C_BRT_WHITE="%B%F{15}"

git_branch() {
	local branch="$(git symbolic-ref --short HEAD 2>/dev/null || git rev-parse --short HEAD 2>/dev/null)"
	if [ -n "${branch}" ]; then
		echo "${branch}"
	fi
}

show_git_branch() {
	if git rev-parse --is-inside-work-tree &>/dev/null; then
		local branch="$(git_branch)"
		if [ -n "${branch}" ]; then
			echo "❮${C_BRT_RED}󰊢 ${C_BRT_MAGENTA}${branch}${C_NORM_YELLOW}❯"
		fi
	fi
}

os_version=$(freebsd-version)
sh_name=$(ps -p $$ -o comm=)
if [ "$(id -u)" -eq 0 ]; then
	usr_color="${C_BRT_RED}"
else
	usr_color="${C_BRT_GREEN}"
fi

export PROMPT=$'
${C_NORM_YELLOW}${C_BRT_RED} ${C_BRT_MAGENTA}${os_version}${C_NORM_YELLOW}─${C_BRT_BLUE} ${C_BRT_MAGENTA}${sh_name}${C_NORM_YELLOW}
${C_NORM_YELLOW}┌──❮ ${C_BRT_GREEN} %*${C_NORM_YELLOW} ❯─❮ ${C_BRT_GREEN} %D{%d/%m/%y}${C_NORM_YELLOW} ❯─❮ ${C_BRT_YELLOW} ${C_BRT_CYAN}%c${C_NORM_YELLOW} ❯─ ❮${C_BRT_BLUE} ${usr_color}%n${C_NORM_YELLOW}❯ $(show_git_branch)
${C_NORM_YELLOW}└─${C_BRT_BLUE}${C_RESET} '

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

# Commands ALIAS
alias clear='printf "\e[H\e[2J\e[3J"'
# Packages ALIAS
alias uppkg='sudo pkg update && sudo pkg upgrade --yes'
alias upall='uppkg'

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################################################################################################################

### ################################
### Installing System Fonts
### ################################

sudo pkg install --yes fontconfig
mkdir -p "${HOME}/.local/share/fonts"

### ################################
### RobotoMono Nerd Fonts
### ################################

# https://www.nerdfonts.com/font-downloads
wget "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/RobotoMono.zip" -O "RobotoMono.zip"
unzip -o RobotoMono.zip -d "${HOME}/.local/share/fonts"
rm -f "${HOME}/.local/share/fonts/LICENSE.txt"
rm -f "${HOME}/.local/share/fonts/README.md"
rm -f RobotoMono.zip

### ################################
### JetBrains Nerd Fonts
### ################################

# https://www.nerdfonts.com/font-downloads
wget "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/JetBrainsMono.zip" -O "JetBrainsMono.zip"
unzip -o JetBrainsMono.zip -d "${HOME}/.local/share/fonts"
rm -f "${HOME}/.local/share/fonts/OFL.txt"
rm -f "${HOME}/.local/share/fonts/README.md"
rm -f JetBrainsMono.zip

### ################################
### MesloLGS Nerd Fonts
### ################################

# https://github.com/romkatv/powerlevel10k#meslo-nerd-font-patched-for-powerlevel10k
# MesloLGS NF Regular
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Regular.ttf" -O "MesloLGS NF Regular.ttf"
mv "MesloLGS NF Regular.ttf" "${HOME}/.local/share/fonts"
# MesloLGS NF Bold
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Bold.ttf" -O "MesloLGS NF Bold.ttf"
mv "MesloLGS NF Bold.ttf" "${HOME}/.local/share/fonts"
# MesloLGS NF Italic
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Italic.ttf" -O "MesloLGS NF Italic.ttf"
mv "MesloLGS NF Italic.ttf" "${HOME}/.local/share/fonts"
# MesloLGS NF Bold Italic
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Bold%20Italic.ttf" -O "MesloLGS NF Bold Italic.ttf"
mv "MesloLGS NF Bold Italic.ttf" "${HOME}/.local/share/fonts"

### ################################
### JetBrains Mono Nerd Fonts
### ################################

# https://github.com/JetBrains/JetBrainsMono
sh -c "$(curl -fsSL https://raw.githubusercontent.com/JetBrains/JetBrainsMono/master/install_manual.sh)"

### ################################
### Nerd Font Symbols Only
### ################################

# https://www.nerdfonts.com/font-downloads
wget "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/NerdFontsSymbolsOnly.zip" -O "NerdFontsSymbolsOnly.zip"
unzip -o "NerdFontsSymbolsOnly.zip" -d "${HOME}/.local/share/fonts"
rm -f "${HOME}/.local/share/fonts/10-nerd-font-symbols.conf"
rm -f "${HOME}/.local/share/fonts/LICENSE"
rm -f "${HOME}/.local/share/fonts/README.md"
rm "NerdFontsSymbolsOnly.zip"

### ################################
### Update Font Cache
### ################################

fc-cache -fv

### ################################################################################################################################

### ################################
### Installing Needed Tools
### ################################

# Man Pages
sudo pkg install --yes mandoc

# Build System
sudo pkg install --yes cmake
sudo pkg install --yes ninja
sudo pkg install --yes meson

# Compress and Decompress
sudo pkg install --yes zip
sudo pkg install --yes unzip
sudo pkg install --yes 7-zip

### ################################
### Installing Rust Tools
### ################################

# New Tools
sudo pkg install --yes eza
sudo pkg install --yes fd-find
sudo pkg install --yes bat
sudo pkg install --yes eza
sudo pkg install --yes grex
sudo pkg install --yes ripgrep

### ################################
### Installing System Fetch
### ################################

# Fetch
sudo pkg install --yes neofetch
sudo pkg install --yes fastfetch
sudo pkg install --yes ufetch
sudo pkg install --yes pfetch-rs
sudo pkg install --yes cpufetch

### ################################
### Installing System Tools
### ################################

# Clipboard
sudo pkg install --yes wl-clipboard
sudo pkg install --yes xclip

### ################################
### Installing Web/Net Tools
### ################################

# Browser
touch "${HOME}/.w3m/history"
sudo pkg install --yes w3m
sudo pkg install --yes lynx
sudo pkg install --yes elinks

# NetCat
sudo pkg install --yes netcat

### ################################
### Installing treeSitter
### ################################

sudo pkg install --yes tree-sitter
sudo pkg install --yes tree-sitter-cli
sudo pkg install --yes tree-sitter-grammars
sudo pkg install --yes tree-sitter-graph

### ################################################################################################################################

### ################################
### Installing Terminal Editor
### ################################

# Terminal Editor
sudo pkg install --yes micro
sudo pkg install --yes nano
sudo pkg install --yes neovim

### ################################
### Installing Window Editor
### ################################

# Window Editor
sudo pkg install --yes emacs

### ################################
### Setup Emacs Config
### ################################

# Remove Lixo
rm -rf "${HOME}/.emacs" 2> "/dev/null"
rm -rf "${HOME}/.emacs.d" 2> "/dev/null"
rm -rf "${HOME}/.config/emacs" 2> "/dev/null"
rm -rf "${HOME}/.config/doom" 2> "/dev/null"

# Setup Doom Emacs
git clone --depth 1 "https://github.com/doomemacs/doomemacs" "${HOME}/.config/emacs"
mkdir -p "${HOME}/.config/doom/snippets"
~/.config/emacs/bin/doom install --force

# Setup Packages
cat << 'EOF' | tee -a "${HOME}/.config/doom/packages.el" > "/dev/null"
(package! mermaid-mode)
(package! ob-mermaid)
EOF
~/.config/emacs/bin/doom sync

# Setup init.el
sed -i 's/;;tree-sitter/tree-sitter/' "${HOME}/.config/doom/init.el"
sed -i 's/;;(cc +lsp)/(cc +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;(rust +lsp)/(rust +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;python/(python +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;javascript/(javascript +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;typescript/(typescript +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;toml/(toml +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;sql/(sql +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/sh[[:space:]]*;/(sh +tree-sitter) ;/' "${HOME}/.config/doom/init.el"
~/.config/emacs/bin/doom sync

# Setup config.el
cat << 'EOF' | tee -a "${HOME}/.config/doom/config.el" > "/dev/null"
;; Configuração de Fonte (JetBrains Mono)
(setq doom-font (font-spec :family "JetBrainsMonoNL Nerd Font Mono" :size 16 :weight 'medium)
      doom-variable-pitch-font (font-spec :family "JetBrainsMonoNL Nerd Font Mono" :size 16))
;; Ativar Cursor Piscante
(blink-cursor-mode t)

;; Configuração Mermaid
(use-package! mermaid-mode
  :mode "\\.mermaid\\'"
  :mode "\\.mmd\\'"
  :config
  (setq mermaid-mmdc-location "mmdc")
  (setq mermaid-output-format "png"))

(use-package! ob-mermaid
  :after org
  :config
  (setq ob-mermaid-cli-path "mmdc"))
EOF
~/.config/emacs/bin/doom sync

# Update Doom Emacs
~/.config/emacs/bin/doom upgrade

### ################################
### Setup NeoVim Config
### ################################

# Remove Lixo
rm -rf "${HOME}/.config/nvim" 2> "/dev/null"

# Setup LazyVim
git clone "https://github.com/LazyVim/starter" "${HOME}/.config/nvim"
rm -rf "${HOME}/.config/nvim/.git"

# Setup options.lua
cat << 'EOF' | tee -a "${HOME}/.config/nvim/lua/config/options.lua" > "/dev/null"
-- Ativar Cursor Piscante
local cursor_gui = vim.api.nvim_get_option_value("guicursor", {})
local cursor_group = vim.api.nvim_create_augroup('ConfigCursor', { clear = true })
vim.api.nvim_create_autocmd({ 'VimEnter', 'VimResume' }, {
	group = cursor_group,
	pattern = '*',
	command = 'set guicursor=' .. cursor_gui .. ',a:blinkwait500-blinkoff500-blinkon500-Cursor/lCursor'
})
vim.api.nvim_create_autocmd({ 'VimLeave', 'VimSuspend' }, {
	group = cursor_group,
	pattern = '*',
	command = 'set guicursor='
})
EOF

### ################################
### Installing Theme in Micro
### ################################

### https://draculatheme.com/micro
git clone "https://github.com/dracula/micro.git"
mkdir -p "${HOME}/.config/micro/colorschemes"
cp "micro/dracula.micro" "${HOME}/.config/micro/colorschemes/dracula.micro"
sudo rm -f -r micro
cat << 'EOF' > "${HOME}/.config/micro/settings.json"
{
	"colorscheme": "dracula"
}
EOF

### ################################################################################################################################

### ################################
### Installing Languages
### ################################

# C/C++
sudo pkg install --yes gcc

# Python
sudo pkg install --yes python

### ################################################################################################################################

# Web Browser
sudo pkg install --yes firefox

# Terminal
sudo pkg install --yes ptyxis

### ################################################################################################################################
