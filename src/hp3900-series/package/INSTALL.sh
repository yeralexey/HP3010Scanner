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

# functions
function header()
{
	# clear screen
	clear

	echo -en $LIGHT_GRAY"hp3900-series $version - INSTALLER\n"
	echo "Created by $AUTHOR <$EMAIL>"
	echo -en $GRAY"\n"
}

function menu_show()
{
	if test $menu_count -gt 0;
	then
		a=0

		echo "$menu_margin""1 - Others (Compile sources)"
		while [ $a -lt $menu_count ]; do
			a=$[$a + 1]
			b=$[$a + 1]
			echo "$menu_margin$b - ${menu_string[$a]}"
		done
	fi
}

function menu_create()
{
	menu_count=0

	for i in $(find "$menu_root" -name "menu");
	do
		# is there a menu option?
		tag="$(cat $i | grep -e "^MENU:" | cut -d: -f2 | sed -e 's/^ *//')"

		if test -n "$tag";
		then
			# seems to be valid, add path
			menu_count=$[$menu_count + 1]

			menu_string[$menu_count]="$tag"
			menu_path[$menu_count]="$(dirname $i)"
			menu_bin[$menu_count]="$(cat $i | grep -e "^BIN:" | cut -d: -f2 | sed -e 's/^ *//')"
			menu_la[$menu_count]="$(cat $i | grep -e "^LA:" | cut -d: -f2 | sed -e 's/^ *//')"
			menu_rules[$menu_count]="$(cat $i | grep -e "^RULES:" | cut -d: -f2 | sed -e 's/^ *//')"
			menu_man[$menu_count]="$(cat $i | grep -e "^MAN:" | cut -d: -f2 | sed -e 's/^ *//')"
		fi
	done;

}

function install_help()
{
	echo
	echo "$0 [arguments]"
	echo
	echo -en $LIGHT_GRAY"Arguments:\n\n"
	echo -en $LIGHT_GRAY"--type <#>: "$GRAY"Selects application to install. Posible values are:\n"
	echo "       1: Stand-alone application"
	echo "       2: SANE Backend"
	echo
	echo -en $LIGHT_GRAY"--distro <#>: "$GRAY"Selects distro to install binaries instead of compiling sources.\n"

	menu_margin="       "

	menu_root=$INS_PATH_BIN_STDALONE
	menu_create

	if test $menu_count -gt "0";
	then
		echo "       Posible values for Stand-alone application:"
		menu_show
	else
		echo
		echo "       There aren't binaries for Stand-alone application. You must compile proyect"
	fi

	# show options for SANE backend
	menu_root=$INS_PATH_BIN_SANE
	menu_create

	if test $menu_count -gt "0";
	then
		echo
		echo "       Posible values for SANE Backend:"
		menu_show
	else
		echo
		echo "       There aren't binaries for SANE backend. You must compile proyect"
	fi

	echo
	echo -en $LIGHT_GRAY"--adistro <#>: "$GRAY"Tries to autodetect system's distro\n"
	echo

	exit 0
}

function installation()
{
	# if install_type is already set, skip this step
	if test $install_type -eq "-1";
	then
		install_type=2; # by default SANE Backend

		echo
		echo -en $GRAY"Select application you wish to install:\n"
		echo "1 - Stand-alone application"
		echo "2 - SANE Backend"
		echo
		echo -en $LIGHT_GRAY"Install type [$install_type]: "$GRAY
		read ins_type

		if test ! -z $ins_type;
		then
			if test $ins_type -gt "0";
			then
				if test $ins_type -lt "3";
				then
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

function distro_detect()
{
	if test -e /etc/suseRegister.conf
	then
		distro=5 # SuSE
	elif test -e /etc/lsb-release;
	then
		grep -i ubuntu /etc/lsb-release > /dev/null
		if test $? -eq 0;
		then
			distro=3 # Ubuntu
		fi
	elif test -e /etc/debian_version;
	then
		distro=2 # Debian
	elif test -e /etc/fedora-release;
	then
		distro=4 # Fedora
	fi
}

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
			echo -en $WARNING"Official binaries not found. Must use compiled one...\n"$GRAY
		fi
	fi
}

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


function stdalone_install()
{
	MSGROOT="Be sure that you execute installation as root"

	case $distro in
	1) # others
		binfile=$INS_PATH_BIN_STDALONE"/others/hp3900"
		kmdrfile=$INS_PATH_BIN_STDALONE"/others/hp3900.kmdr"
		manfile=$INS_PATH_BIN_STDALONE"/others/hp3900.1"
		rulesfile=$INS_PATH_BIN_STDALONE"/others/hp3900.rules"

		# check if binary already exists. If don't, compile it
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
