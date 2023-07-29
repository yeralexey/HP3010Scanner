#!/bin/bash

# read version of hp3900-series
. ./scripts/version.sh

# read config
. ./scripts/config.sh

# read colors
. ./scripts/colors.sh

# menu variables
declare -a menu_path
declare -a menu_string
declare -a menu_bin
declare -a menu_la
declare -a menu_rules
declare -a menu_man
menu_op=0
menu_count=0
menu_root=""
menu_margin=""

## FUNCTIONS

# №1. Displaying a header at the beginning of the installation script.
# The purpose is to provide a visually appealing and informative header at the beginning of the script's execution
function header()
{
	# clear screen
	clear

	echo -en $LIGHT_GRAY"hp3900-series $version - INSTALLER\n"
	echo "Created by $AUTHOR <$EMAIL>"
	echo -en $GRAY"\n"
}


# №2. Essentially displays the available menu options for installation, 
# showing their corresponding numbers and names. 
function menu_show()
{
	# check if flag of menu count is freater than 0
	if test $menu_count -gt 0; 
	then
		a=0

		# prints first menu option
		echo "$menu_margin""1 - Others (Compile sources)"
		# iterates over availiable menu options
		while [ $a -lt $menu_count ]; do
			a=$[$a + 1]
			b=$[$a + 1]
			echo "$menu_margin$b - ${menu_string[$a]}"
		done
	fi
}


# №3. Initializes several arrays (menu_string, menu_path, menu_bin, menu_la, menu_rules, menu_man) 
# and a counter variable menu_count.
function menu_create()
{
	menu_count=0

	# use find command to search for files named "menu" within the directory specified by the variable $menu_root
	for i in $(find "$menu_root" -name "menu");
	do
		# is there a menu option?
		# for each "menu" file found, read the content of the file and 
		# extracts specific information using grep, cut, and sed commands.
		tag="$(cat $i | grep -e "^MENU:" | cut -d: -f2 | sed -e 's/^ *//')"

		# conditional statement that checks whether the variable $tag is non-empty (i.e., has a value)
		if test -n "$tag";
		then
			# seems to be valid, add path
			menu_count=$[$menu_count + 1]

			# the name or description of the menu option
			menu_string[$menu_count]="$tag"
			# the path of the directory containing the "menu" file
			menu_path[$menu_count]="$(dirname $i)"
			# the name of the binary file associated with the menu option
				# syntax break down:
				# `menu_bin[$menu_count]=:` assigns a value to the array element menu_bin at the index specified by the value of the variable menu_count
				# `$(...):` a command substitution. The enclosed commands are executed, and their output is substituted back into the main command
				# `cat $i:` is used to read the contents of the file specified by the variable $i.
				# `grep -e "^BIN:":` searches for lines in the output of cat $i that start with the string "BIN:", `-e`` option is used to specify the pattern to search for
				# `cut -d: -f2:` extracts the second field from each line in the output of grep, `-d:` option sets the delimiter to ":" (colon), and the -f2 option specifies to select the second field
				# `sed -e 's/^ *//':` removes leading spaces from the output of cut, regular expression 's/^ *//' matches and substitutes any leading spaces at the beginning of each line with an empty string
			# the name of the binary file associated with the menu option
			menu_bin[$menu_count]="$(cat $i | grep -e "^BIN:" | cut -d: -f2 | sed -e 's/^ *//')"
			# the name of the library file associated with the menu option
			menu_la[$menu_count]="$(cat $i | grep -e "^LA:" | cut -d: -f2 | sed -e 's/^ *//')"
			# the name of the rules file associated with the menu option
			menu_rules[$menu_count]="$(cat $i | grep -e "^RULES:" | cut -d: -f2 | sed -e 's/^ *//')"
			# the name of the man page file associated with the menu option
			menu_man[$menu_count]="$(cat $i | grep -e "^MAN:" | cut -d: -f2 | sed -e 's/^ *//')"
		fi
	done;

}

# №4. Displaying the help menu with information about the script's usage and available command-line arguments, 
# provides details on how to use the script and what options are available for the installation process.
function install_help()
{
	# print emty line
	echo
	# display current (./INSTALL.sh )script name followed by word `arguments` in square brackets
	echo "$0 [arguments]"
	echo
	#  print the section header for the arguments in light gray color, followed by two new lines., etc.
	echo -en $LIGHT_GRAY"Arguments:\n\n"
	echo -en $LIGHT_GRAY"--type <#>: "$GRAY"Selects application to install. Posible values are:\n"
	echo "       1: Stand-alone application"
	echo "       2: SANE Backend"
	echo
	echo -en $LIGHT_GRAY"--distro <#>: "$GRAY"Selects distro to install binaries instead of compiling sources.\n"

	# set the menu_margin variable to add indentation in the menu display
	menu_margin="       "

	# set menu_root variable to the path of the stand-alone application binaries
	menu_root=$INS_PATH_BIN_STDALONE
	# call menu_create function to update menu_count variable
	menu_create

	# checks if there are any stand-alone application binaries available by cheching menu_count variable
	if test $menu_count -gt "0";
	then
		echo "       Posible values for Stand-alone application:"
		menu_show
	else
		echo
		# TODO could be a mistake, because i receive this string even with compiles binnaries
		echo "       There aren't binaries for Stand-alone application. You must compile project"
	fi

	# show options for SANE backend, same to previous
	menu_root=$INS_PATH_BIN_SANE
	menu_create

	if test $menu_count -gt "0";
	then
		echo
		echo "       Posible values for SANE Backend:"
		menu_show
	else
		echo
		echo "       There aren't binaries for SANE backend. You must compile project"
	fi

	echo
	echo -en $LIGHT_GRAY"--adistro <#>: "$GRAY"Tries to autodetect system's distro\n"
	echo

	exit 0
}

# №5. Handling the user's selection of the installation type (stand-alone application or SANE Backend), 
# if the install_type is not already set, it prompts the user to choose between the two options,
# sets the install_type variable based on the user's input.
function installation()
{
	# if install_type is already set, skip this step
	if test $install_type -eq "-1";
	then
		install_type=1; # by default Stand-alone backend

		echo
		echo -en $GRAY"Select application you wish to install:\n"
		echo "1 - Stand-alone application"
		echo "2 - SANE Backend"
		echo
		echo -en $LIGHT_GRAY"Install type [$install_type]: "$GRAY

		# reads the user's input and stores it in the variable ins_type
		read ins_type

		# checks if the user's input is not empty
		if test ! -z $ins_type;
		then
			# checks if the user's input is greater than zero
			if test $ins_type -gt "0";
			then
				# checks if the user's input is less than 3
				if test $ins_type -lt "3";
				then
					# sets the install_type variable to the user's input, which represents the chosen installation type
					install_type=$ins_type
				fi
			fi
		fi
	fi

	# set proper path to detect available binary path
	case $install_type in
	1) menu_root=$INS_PATH_BIN_STDALONE ;;
	*) menu_root=$INS_PATH_BIN_SANE ;;
	esac
}

# №6. Is responsible for determining the Linux distribution running on the system, checks for specific files or 
# directories commonly found on different Linux distributions to identify the distribution type.
function distro_detect()
{
	# file is commonly found on SuSE (openSUSE) Linux systems, if exists, distro variable is set to 5
	if test -e /etc/suseRegister.conf
	then
		distro=5 # SuSE
	# if the file exists and contains the word "ubuntu" (case-insensitive), distro variable is set to 3
	elif test -e /etc/lsb-release;
	then
		grep -i ubuntu /etc/lsb-release > /dev/null
		if test $? -eq 0;
		then
			distro=3 # Ubuntu
		fi
	#  if the file /etc/debian_version exists...
	elif test -e /etc/debian_version;
	then
		distro=2 # Debian
	#  if the file /etc/fedora-release exists...
	elif test -e /etc/fedora-release;
	then
		distro=4 # Fedora
	fi
}


# №7. Is responsible for allowing the user to select a Linux distribution for installation, used to determine 
# whether pre-compiled binaries for the specified distribution are available or if the user needs to compile the sources.
function distro_select()
{
	# first generate menu to get proper config about desired application
	menu_margin=""
	menu_create

	# check if $distro has a proper value
	if test $distro -ne "-1";
	then
		if test $distro -gt "0";
		then
			if test $distro -lt $[$menu_count + 2];
			then
				menu_op=$[$distro - 1]
			else
				echo
				echo -en $WARNING"Selected binaries don't exist in this package...\n"$GRAY
				distro=-1
			fi
		else
			echo
			echo -en $WARNING"Selected binaries don't exist in this package...\n"$GRAY
			distro=-1
		fi
	fi

	if test $distro -eq "-1";
	then
		# default option Compile
		distro=1
		menu_op=0

		# are there binaries ready to install?
		if test $menu_count -gt "0";
		then
			echo
			echo -en $GRAY"There are binaries for some distros to skip compilation process.\n"
			echo "If your distro isn't listed below, select 'others' option to compile sources."
			echo

			menu_show

			echo
			echo -en $LIGHT_GRAY"Your distro : "$GRAY
			read my_distro

			if test ! -z $my_distro;
			then
				if test $my_distro -gt "0";
				then
					if test $my_distro -lt $[$menu_count + 2];
					then
						distro=$my_distro
						menu_op=$[$distro - 1]
					fi
				fi
			fi
		else
			# compile project
			echo
			# Thats what i receive
			echo -en $WARNING"test Official binaries not found. Must use compiled one...\n"$GRAY
		fi
	fi
}


# №8. Checking whether the required folders for installing the SANE backend exist on the system, 
# provides information about the expected paths and verifies if they exist.
function sane_detect_folders()
{
	check=0

	echo -en $INFO"Installation supposes you've already installed SANE in your system\n"
	echo -en $ACTION"Checking that path variables point to the right places ...\n"

	if !(test -e $SNE_PATH_LIBS);
	then
		echo -en $ERROR"$SNE_PATH_LIBS does NOT exist. Check variable SNE_PATH_LIBS at ./scripts/config.sh\n"
		check=1
	fi

	if !(test -e $SNE_PATH_CFG);
	then
		echo -en $ERROR"$SNE_PATH_CFG does NOT exist. Check variable SNE_PATH_CFG at ./scripts/config.sh\n"
		check=1
	fi

	# if errors detected, exit
	if test $check -eq "1"
	then
		exit 1
	fi
}

# №9. Installing the SANE backend after 
# compilation.
function sane_install_compilation()
{
	sPATH=$INS_PATH_BIN_SANE/others

	# check if binary already exists. If don't, compile it
	if !(test -e $sPATH/libsane-hp3900.la);
	then
		echo -en $WARNING"Binaries not found. Must compile first...\n"$GRAY
		./COMPILE.sh --type 2 --noheader
		if test $? -ne "0";
		then
			# compile failed, exit
			echo -en $ERROR"Aborting installation process ...\n"
			exit 1
		fi
	fi

	echo -en $LIGHT_GRAY"- Starting installation of SANE backend...\n"$GRAY

	# get sane version
	SANE_VERSION=$(basename $sPATH/*so.*.*.*|cut -c19-)

	# get lib path
	LIB_PATH=$(cat $sPATH/libsane*.la|grep libdir|cut -b 8-|tr -d \')

	if test -z $LIB_PATH;
	then
		LIB_PATH=$SNE_PATH_LIBS
	fi

	# copy libraries
	echo -en $ACTION"Copying libraries at $LIB_PATH/ ...\n"
	cp $sPATH/libsane*.la $LIB_PATH/ > /dev/null
	cp $sPATH/libsane*.so.* $LIB_PATH/ > /dev/null

	# create symlinks
	echo -en $ACTION"Creating symlinks at $LIB_PATH/ ...\n"
	if test -e $LIB_PATH/libsane-hp3900.so.1;
	then
		rm -rf $LIB_PATH/libsane-hp3900.so.1 > /dev/null
	fi

	if test -e $LIB_PATH/libsane-hp3900.so;
	then
		rm -rf $LIB_PATH/libsane-hp3900.so > /dev/null
	fi

	ln -sf $LIB_PATH/libsane-hp3900.so.$SANE_VERSION $LIB_PATH/libsane-hp3900.so.1 > /dev/null
	ln -sf $LIB_PATH/libsane-hp3900.so.1 $LIB_PATH/libsane-hp3900.so > /dev/null

	# copy man file
	echo -en $ACTION"Copying man file at $SNE_PATH_MAN/ ...\n"
	cp $sPATH/sane-hp3900.5 $SNE_PATH_MAN > /dev/null

	# copy udev rules file
	echo -en $ACTION"Copying UDEV rules file at $STD_PATH_UDEV/ ...\n"
	cp $sPATH/libsane.rules $STD_PATH_UDEV > /dev/null

	# create symlinks at $STD_PATH_UDEV/rules.d
	echo -en $ACTION"Creating symlinks at $STD_PATH_UDEV/rules.d/ ...\n"
	if test -e $STD_PATH_UDEV/rules.d/025_libsane.rules;
	then
		rm -rf $STD_PATH_UDEV/rules.d/025_libsane.rules > /dev/null
	fi

	ln -sf $STD_PATH_UDEV/libsane.rules $STD_PATH_UDEV/rules.d/025_libsane.rules > /dev/null

	# copy hp3900.conf at $SNE_PATH_CFG
	echo -en $ACTION"Copying hp3900.conf file at $SNE_PATH_CFG/ ...\n"
	cp $sPATH/hp3900.conf $SNE_PATH_CFG > /dev/null

	# setup dll.conf
	echo -en $ACTION"Setting up $SNE_PATH_CFG/dll.conf ...\n"

	if test -e $SNE_PATH_CFG/dll.conf;
	then
		#if exists line, modify it else add it
		if grep -q '^#hp3900' $SNE_PATH_CFG/dll.conf
		then
			# hp3900 exists but is disabled
			echo -en $ACTION"Enabling hp3900 backend in dll.conf ...\n"
			cat $SNE_PATH_CFG/dll.conf | sed -e 's/^#hp3900/hp3900/' > $SNE_PATH_CFG/dll.conf
		else
			if ! grep -q '^hp3900' $SNE_PATH_CFG/dll.conf
			then
				echo -en $ACTION"Adding hp3900 backend in dll.conf ...\n"
				echo "hp3900" >> $SNE_PATH_CFG/dll.conf
			else
				echo -en $INFO"hp3900 backend already enabled in dll.conf ...\n"
			fi
		fi
	else
		# dll.conf doesn't exist. Lets create it
		echo -en $ACTION"Creating dll.conf ...\n"
		echo "hp3900" > $SNE_PATH_CFG/dll.conf
	fi

	echo -en $LIGHT_GRAY"- Installation ends succesfully...\n"$GRAY
}

# №10. handles the installation of the SANE backend, 
# begins by checking path variables using the sane_detect_folders() function.
function sane_install()
{
	# lets check path variables first
	sane_detect_folders

	if test $distro -ne "1";
	then
		commonpath=$INS_PATH_BIN_SANE/common
		binfile=$commonpath/libsane-hp3900.so.*.*.*
		lafile=$commonpath/libsane-hp3900.la
		manfile=$commonpath/sane-hp3900.5
		rulesfile=$commonpath/libsane.rules

		# specific paths
		if test $(echo ${menu_bin[$menu_op]} | wc -c) -gt "1";
		then
			binfile=${menu_path[$menu_op]}"/"${menu_bin[$menu_op]}
		fi

		if test $(echo ${menu_la[$menu_op]} | wc -c) -gt "1";
		then
			lafile=${menu_path[$menu_op]}"/"${menu_la[$menu_op]}
		fi

		if test $(echo ${menu_man[$menu_op]} | wc -c) -gt "1";
		then
			manfile=${menu_path[$menu_op]}"/"${menu_man[$menu_op]}
		fi

		if test $(echo ${menu_rules[$menu_op]} | wc -c) -gt "1";
		then
			rulesfile=${menu_path[$menu_op]}"/"${menu_rules[$menu_op]}
		fi

		echo -en $LIGHT_GRAY"- Starting installation of SANE backend...\n"$GRAY

		# get sane version
		SANE_VERSION=$(basename $binfile|cut -c19-)

		# get lib path
		LIB_PATH=$(cat $lafile|grep libdir|cut -b 8-|tr -d \')

		if test -z $LIB_PATH;
		then
			LIB_PATH=$SNE_PATH_LIBS
		fi

		# copy libraries
		echo -en $ACTION"Copying libraries at $LIB_PATH ...\n"
		cp $lafile $LIB_PATH/ > /dev/null
		cp $binfile $LIB_PATH/ > /dev/null

		# create symlinks
		echo -en $ACTION"Creating symlinks at $LIB_PATH ...\n"
		if test -e $LIB_PATH/libsane-hp3900.so.1;
		then
			rm -rf $LIB_PATH/libsane-hp3900.so.1 > /dev/null
		fi

		if test -e $LIB_PATH/libsane-hp3900.so;
		then
			rm -rf $LIB_PATH/libsane-hp3900.so > /dev/null
		fi

		ln -sf $LIB_PATH/libsane-hp3900.so.$SANE_VERSION $LIB_PATH/libsane-hp3900.so.1 > /dev/null
		ln -sf $LIB_PATH/libsane-hp3900.so.1 $LIB_PATH/libsane-hp3900.so > /dev/null

		# copy man file
		echo -en $ACTION"Copying man file at $SNE_PATH_MAN ...\n"
		cp $manfile $SNE_PATH_MAN > /dev/null

		# copy udev rules file
		echo -en $ACTION"Copying UDEV rules file at $STD_PATH_UDEV ...\n"
		cp $rulesfile $STD_PATH_UDEV > /dev/null

		# create symlinks at $STD_PATH_UDEV/rules.d
		echo -en $ACTION"Creating symlinks at $STD_PATH_UDEV/rules.d/ ...\n"
		if test -e $STD_PATH_UDEV/rules.d/025_libsane.rules;
		then
			rm -rf $STD_PATH_UDEV/rules.d/025_libsane.rules > /dev/null
		fi

		ln -sf $STD_PATH_UDEV/libsane.rules $STD_PATH_UDEV/rules.d/025_libsane.rules > /dev/null

		# copy hp3900.conf at $SNE_PATH_CFG
		echo -en $ACTION"Copying hp3900.conf file at $SNE_PATH_CFG/ ...\n"
		cp $commonpath/hp3900.conf $SNE_PATH_CFG > /dev/null

		# setup dll.conf
		echo -en $ACTION"Setting up $SNE_PATH_CFG/dll.conf ...\n"

		if test -e $SNE_PATH_CFG/dll.conf;
		then
			#if exists line, modify it else add it
			if grep -q '^#hp3900' $SNE_PATH_CFG/dll.conf
			then
				# hp3900 exists but is disabled
				echo -en $ACTION"Enabling hp3900 backend in dll.conf ...\n"
				cat $SNE_PATH_CFG/dll.conf | sed -e 's/^#hp3900/hp3900/' > $SNE_PATH_CFG/dll.conf
			else
				if ! grep -q '^hp3900' $SNE_PATH_CFG/dll.conf
				then
					echo -en $ACTION"Adding hp3900 backend in dll.conf ...\n"
					echo "hp3900" >> $SNE_PATH_CFG/dll.conf
				else
					echo -en $INFO"hp3900 backend already enabled in dll.conf ...\n"
				fi
			fi
		else
			# dll.conf doesn't exist. Lets create it
			echo -en $ACTION"Creating dll.conf ...\n"
			echo "hp3900" > $SNE_PATH_CFG/dll.conf
		fi

		echo -en $LIGHT_GRAY"- Installation ends succesfully...\n"$GRAY
	else
		sane_install_compilation
	fi
}

# №11. The stdalone_install() function copies the necessary files of the stand-alone application to their 
# respective installation directories. It handles two scenarios: when the binaries are compiled from source 
# (distro=1) and when they are pre-compiled (distro>1). It checks for the existence of the binary 
# file and copies it to the appropriate destination. It also copies the man page and udev rules, 
# and creates a symlink for the udev rules file.
function stdalone_install()
{
	# a variable containing a message reminding the user to execute the installation as root
	MSGROOT="Be sure that you execute installation as root"

	# case statement to handle two scenarios based on the distro value
	case $distro in
	1) # others
		#for the case where the stand-alone application needs to be compiled from source, the paths for the binary, 
		# kmdr file, man file, and udev rules file are set based on the paths specified in the config.sh file.
		binfile=$INS_PATH_BIN_STDALONE"/others/hp3900"
		kmdrfile=$INS_PATH_BIN_STDALONE"/others/hp3900.kmdr"
		manfile=$INS_PATH_BIN_STDALONE"/others/hp3900.1"
		rulesfile=$INS_PATH_BIN_STDALONE"/others/hp3900.rules"


		# check if binary already exists, but option force recompile is enabled
		# trigger the compilation process by running ./COMPILE.sh --type 1 --noheader (added modification)

		if [ -e "$binfile" ]; then
			if [ "$force_recompile" -gt 0 ]; then # force_recompile can be found in ./scripts/config.sh
				echo -en "$WARNING""Binaries were found, but forced recompilation is enabled,\
				 check ./scripts/config.sh for more, recompiling...\n""$GRAY"
				./COMPILE.sh --type 1 --noheader

				if [ $? -ne "0" ]; then
					# compile failed, exit
					echo -en "$ERROR""Aborting installation process ...\n"
					exit 1
				fi
			fi
		fi

		# check if binary don't exists, compile it, trigger the compilation process by running 
		# ./COMPILE.sh --type 1 --noheader (inherited from original code)

		if !(test -e $binfile);
		then
			echo -en $WARNING"Binaries not found. Must compile first...\n"$GRAY
			./COMPILE.sh --type 1 --noheader
			if test $? -ne "0";
			then
				# compile failed, exit
				echo -en $ERROR"Aborting installation process ...\n"
				exit 1
			fi
		fi
		;;


	*) # compiled binaries
		# default paths
		binfile=$INS_PATH_BIN_STDALONE"/common/hp3900"
		kmdrfile=$INS_PATH_BIN_STDALONE"/common/hp3900.kmdr"
		manfile=$INS_PATH_BIN_STDALONE"/common/hp3900.1"
		rulesfile=$INS_PATH_BIN_STDALONE"/common/hp3900.rules"

		# specific paths
		if test $(echo ${menu_bin[$menu_op]} | wc -c) -gt "1";
		then
			binfile=${menu_path[$menu_op]}"/"${menu_bin[$menu_op]}
		fi

		if test $(echo ${menu_man[$menu_op]} | wc -c) -gt "1";
		then
			manfile=${menu_path[$menu_op]}"/"${menu_man[$menu_op]}
		fi

		if test $(echo ${menu_rules[$menu_op]} | wc -c) -gt "1";
		then
			rulesfile=${menu_path[$menu_op]}"/"${menu_rules[$menu_op]}
		fi
		;;
	esac

	echo
	echo -en $LIGHT_GRAY"- Starting installation of stand-alone application...\n"$GRAY

	# copy binary file
	if test -e $binfile;
	then
		echo -en $ACTION"Copying binary application at $STD_PATH_BINARY/ ...\n"

		cp $kmdrfile $STD_PATH_BINARY 2> /dev/null
		cp $binfile $STD_PATH_BINARY/hp3900 2> /dev/null

		if test $? -eq "0";
		then
			# copy man file
			if test -e $manfile;
			then
				echo -en $ACTION"Copying man file at $STD_PATH_MAN/ ...\n"
				cp $manfile $STD_PATH_MAN/  > /dev/null

				if test $? -eq "0";
				then
					# copy udev rules file
					echo -en $ACTION"Copying UDEV rules file at $STD_PATH_UDEV/ ...\n"
					cp $rulesfile $STD_PATH_UDEV > /dev/null

					if test $? -eq "0";
					then
						# create sym link to udev rules file
						echo -en $ACTION"Creating symlink to UDEV rules file at $STD_PATH_UDEV/rules.d ...\n"

						if test -e $STD_PATH_UDEV/rules.d/025_hp3900.rules;
						then
							rm $STD_PATH_UDEV/rules.d/025_hp3900.rules
						fi

						ln -sf $STD_PATH_UDEV/hp3900.rules $STD_PATH_UDEV/rules.d/025_hp3900.rules > /dev/null

						if test $? -eq "0";
						then
							echo -en $LIGHT_GRAY"- Installation ends succesfully...\n"$GRAY
						else
							echo -en $ERROR"Could not create symlink to $STD_PATH_UDEV/hp3900.rules at $STD_PATH_UDEV/rules.d/. $MSGROOT.\n"
						fi
					else
						echo -en $ERROR"Could not create file at $STD_PATH_UDEV. $MSGROOT.\n"
					fi
				else
					echo -en $ERROR"Could not create file at $STD_PATH_MAN. $MSGROOT.\n"
				fi
			else
				echo -en $ERROR"File $(basename $manfile) not found at $(dirname $manfile)/\n"
			fi
		else
			echo -en $ERROR"Could not create file at $STD_PATH_BINARY. $MSGROOT.\n"
		fi
	else
		echo -en $ERROR"File $(basename $binfile) not found at $(dirname $binfile)/\n"
	fi
}

## MAIN SEQUENCE

# show installer header
header

# parse arguments
setting=0

while (( $# ))
do
	if test $setting -gt 0;
	then
		# an argument setting
		case $setting in
		1) # type
			if test $1 -gt "0";
			then
				if test $1 -lt "3";
				then
					install_type=$1
				fi
			fi;;
		2) # distro
			distro=$1 ;;
		esac

		# reset setting status
		setting=0;
	else
		# another argument
		case $1 in
		--help )
			install_help;;
		--type )
			setting=1;;
		--distro )
			setting=2;;
		--adistro )
			distro_detect;;
		*)
			echo "Unknown '$1' option. Type $0 --help"
			exit 1;;
		esac
	fi

	shift
done

# install process must be performed by root
if test $(whoami) != "root";
then
	echo "You must be logged as root to perform install process!"
	echo
	exit 1
fi

# user must select application to install
installation

# user can select between installing an already compiled binary or compiling sources
distro_select

case $install_type in
1) # hp3900
	stdalone_install;;
*) # sane
	sane_install;;
esac

# exit setup
exit 0
