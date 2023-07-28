#!/bin/bash

curdir=$(pwd)
fecha=$(date +%d%m%y-%H%M)
path=~/hp3900-series-$fecha
compress=0

function show_header()
{
	echo "hp3900-series package generator"
	echo "Created by Jonathan Bravo Lopez <jkdsoft@gmail.com>"
	echo
}

function show_help()
{
	echo
	echo "$0 [arguments]"
	echo
	echo "Arguments:\n\n"
	echo "-a : Shows application header"
	echo "-c : Generates a compressed package"
	echo "-h : Shows this help"
	echo

	exit 0
}

function create_pack()
{
	# create path
	mkdir -p $path

	if test ! $? -eq "0";
	then
		echo "Couldn't create path $path . Exiting..."
		exit 1
	fi

	# copy scripts
	cp ./package/*.* $path/
	chmod +x $path/*.sh

	mkdir -p $path/scripts

	if test $? -eq "0";
	then
		cp ./package/scripts/*.sh $path/scripts
		chmod +x $path/scripts/*.sh
	else
		echo "Couldn't create path $path/scripts . Skipping step..."
	fi

	# create neccesary folders
	mkdir -p $path/bin/sane/common
	mkdir -p $path/bin/sane/others
	mkdir -p $path/bin/stdalone/common
	mkdir -p $path/bin/stdalone/others

	mkdir -p $path/src
	mkdir -p $path/log

	# copy source project to src folder
	cp -R $curdir $path/src/hp3900-series

	if test ! $? -eq "0";
	then
		echo "Couldn't copy svn files at $path/src/"
	fi

	# remove some cvs/svn folders
	for i in $(find "$path/src/hp3900-series" -name ".svn");
	do
		rm -rf $i
	done;

	# compile patcher
	cd $path/src/hp3900-series/patcher
	./compile.sh > /dev/null
	cd $curdir

	# create compressed tar file and erase folder ?
	if test $compress -eq "1";
	then
		cd ~
		tar cfjP hp3900-series.tar.bz2 hp3900-series-$fecha

		if test $? -eq "0"
		then
			rm -rf hp3900-series-$fecha

			echo "Created hp3900-series.tar.bz2"
		else
			echo "Couldn't compress directory ~/hp3900-series-$fecha. It remains available."
		fi

		cd $curdir
	else
		echo "Created ~/hp3900-series-$fecha"
	fi
}

# parse arguments
setting=0

while (( $# ))
do
	if test $setting -gt 0;
	then
		# an argument setting
#		case $setting in
#		1) # type
#			if test $1 -gt "0";
#			then
#				if test $1 -lt "3";
#				then
#					install_type=$1
#				fi
#			fi;;
#		2) # distro
#			distro=$1 ;;
#		esac

		# reset setting status
		setting=0;
	else
		# another argument
		case $1 in
		-a ) show_header;;
		-h ) show_help;;
		-c ) compress=1;;
		*)
			echo "Unknown '$1' option. Type $0 -h"
			exit 1;;
		esac
	fi

	shift
done

create_pack

exit 0