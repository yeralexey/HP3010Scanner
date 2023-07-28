#!/bin/bash

# read version of hp3900-series
. ./scripts/version.sh

# read config
. ./scripts/config.sh

# read colors
. ./scripts/colors.sh

# variables
system=0  # 0:32 bit | 1:64 bit
distro="unknown"
mdistro=""
kernel=$(uname -a)
comp1=0 # standalone compilation 0:ok 1:error
comp2=0 # sane compilation 0:ok 1:error
curpath=$(pwd)
fecha=$(date +%d%m%y-%H%M)
destfile="hp3900-pack-$fecha.tar.bz2"

function distro_detect()
{
	if test -e /etc/suseRegister.conf
	then
		distro="SuSE" # SuSE
	elif test -e /etc/lsb-release;
	then
		grep -i ubuntu /etc/lsb-release > /dev/null
		if test $? -eq 0;
		then
			distro="Ubuntu" # Ubuntu
		fi
	elif test -e /etc/debian_version;
	then
		distro="Debian" # Debian
	elif test -e /etc/fedora-release;
	then
		distro="Fedora" # Fedora
	fi
}

function distro_get()
{
	distro_detect

	echo "I would need to know which distro you are using."

	r=1
	while [ $r -eq "1" ]; do
		echo "Type the name of your distro (for example: Debian, Ubuntu, Fedora, SuSE...)"
		echo -en $LIGHT_GRAY"Distro: "$GRAY
		read a

		if test ! -z $a;
		then
			mdistro=$a
			r=0
		else
			mdistro=$distro
			echo "Your distro seems to be $distro. Is it correct? [Y/n]"
			read a

			if test ! -z $a;
			then
				a=$(echo $a|tr "[:upper:]" "[:lower:]")
				if test $a = "y";
				then
					r=0
				fi
			else
				r=0
			fi
		fi
	done
}

function pack()
{
	echo "Packaging binaries and information ..."
	cd $curpath/tmp

	tar cfjP $destfile pack

	if test $? -eq "0";
	then
		mv $destfile $curpath

		echo "Created packed file $destfile"
	else
		echo -en $ERROR"Couldn't create packed file\n"
	fi

	cd $curpath
}

function compile()
{
	echo "Compiling process is starting now. Please, be patient ..."

	echo "- Compiling stand-alone application ..."
	# compile stand-alone application
	./COMPILE.sh --type 1 --noheader --quiet
	comp1=$?

	if test $comp1 -eq "1";
	then
		echo "It seems that there was an error compiling stand-alone application."
	fi

	echo "- Compiling sane backend ..."
	./COMPILE.sh --type 2 --noheader --quiet
	comp2=$?

	if test $comp2 -eq "1";
	then
		echo "It seems that there was an error compiling SANE backend."
	fi

	# copy files
	if test -e $curpath/bin/stdalone/others;
	then
		mkdir -p $curpath/tmp/pack/stdalone
		cp $curpath/bin/stdalone/others/* $curpath/tmp/pack/stdalone
	fi

	if test -e $curpath/bin/sane/others;
	then
		mkdir -p $curpath/tmp/pack/sane
		cp $curpath/bin/sane/others/* $curpath/tmp/pack/sane
	fi
}

function log_write()
{
	path=$curpath/tmp/pack/pack.txt

	echo "Date: "$(date) > $path
	echo "Distro: $distro" >> $path
	echo "MDistro: $mdistro" >> $path
	echo "Kernel: $kernel" >> $path
	echo "ls file type:"$(file /bin/ls) >> $path
	echo >> $path
	./INFO.sh --noheader >> $path
	echo >> $path
	echo "Stand-alone compiling status: $comp1" >> $path
	echo "SANE compiling status: $comp2" >> $path
	
}

function ftp_send()
{
	SEND=1
	USER="jkdftp"
	PASS="guest"
	DIRECTORY="upload"
	HOST="jkdsoftware.dyndns.org"

	echo "If you are online, I can try to send $destfile to developer using ftp."
	echo "Do you want me to try to send file? [Y/n]"
	read a

	if test ! -z $a;
	then
		a=$(echo $a|tr "[:upper:]" "[:lower:]")
		if test $a = "y";
		then
			SEND=0
		else
			echo "You can send $destfile by mail to $EMAIL"
		fi
	else
		SEND=0
	fi

	if test $SEND -eq "0";
	then
	echo "Trying to send file using FTP"
	echo "If you see any error message, you can send $destfile by mail to $EMAIL"
# do not insert any tab or space befor each next line
ftp -n $HOST <<EOF
quote user $USER
quote pass $PASS
binary
cd $DIRECTORY
put $destfile
quit
EOF
	fi
}

function dir_create()
{
	if test -e ./tmp/pack;
	then
		rm -rf ./tmp/pack
	fi

	mkdir -p ./tmp/pack
}

function dir_delete()
{
	rm -rf ./tmp/pack
}

function steps()
{
	echo "Hi hp3900-series user,"
	echo "This script has been written to help you to compile sources,"
	echo "to package resulting binaries and to send package to project's"
	echo "developer, so binaries can be added in the official release in"
	echo "few steps."
	echo
	echo "Do you want to continue? [Y/n]"
	read a

	if test -z $a;
	then
		a="y"
	else
		a=$(echo $a|tr "[:upper:]" "[:lower:]")
	fi

	if test $a = "y";
	then
		# create working directory
		dir_create

		echo "STEP 1:"
		compile

		echo
		echo "STEP 2:"
		distro_get
		log_write

		echo
		echo "STEP 3:"
		pack

		echo
		echo "STEP 4:"
		ftp_send

		# delete working directory
		dir_delete
	fi

	exit 0
}

steps

# exit setup
exit 0
