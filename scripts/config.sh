#!/bin/bash

# path to install files

# stand-alone binary application and man file
STD_PATH_BINARY="/usr/bin"
STD_PATH_MAN="/usr/share/man/man1"
STD_PATH_UDEV="/etc/udev"

# sane library, man file and config files
SNE_PATH_LIBS="/usr/lib/sane"
SNE_PATH_LIBS64="/usr/lib64/sane"
SNE_PATH_MAN="/usr/share/man/man5"
SNE_PATH_CFG="/etc/sane.d"



# Next lines shouldn't be edited unless you know what you are doing !!!



# paths for hp3900-series installer
INS_PATH_BIN_STDALONE="./bin/stdalone"
INS_PATH_BIN_SANE="./bin/sane"

UPD_PATH_SRC_STDALONE="./src/hp3900-series"
UPD_PATH_SRC_SANE="./src/sane-backends"

UPD_PATH_TMP_STDALONE="./src/hp3900_temp"
UPD_PATH_TMP_SANE="./src/sane_temp"

CMP_LOG_SANE="sanecomp.log"

# default values for next processes
distro=-1       # 1=none, 2=debian, 3=ubuntu, 4=fedora, 5=suse
install_type=-1 # 1=hp3900, 2=SANE
update_type=-1  # 1=hp3900, 2=SANE, 3=Both
compile_type=-1 # 1=hp3900, 2=SANE, 3=Both
info_type=0     # 0=All, 1=hp3900_bin_vrs, 2=hp3900_src_vrs, 3=Sane_src_vrs