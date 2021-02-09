validate_backup_params(){
	if [[ $# -eq 0 ]]; then
       		echo -e "Error: Not Enough Arguments\nFirst Argument: Path to the directory to be backed up\nSecond Argument: Path to the directory to store the back-up inside\nThird Argument: the encryption key"
		return 0	
	elif [[ $# -lt 3 ]]; then
		echo "Error: Not Enough Arguments"
		return 0
	elif [[ $# -gt 3 ]]; then
		echo "Error: Arguments exceeded 3"
		return 0
	fi

	tobe_back_up_dir=$1
	back_up_store_dir=$2

	if [[ ! -d $tobe_back_up_dir ]]; then
		echo "The directory to be backed up is not a valid one"
		return 0
	elif [[ ! -d $back_up_store_dir ]]; then
		echo "The directory to store the back up inside is not valid"
		return 0
	fi
	return 1
}
validate_restore_params(){
	if [[ $# -eq 0 ]]; then
       		echo -e "Error: Not Enough Arguments\nFirst Argument: Path to the directory that contains the backup\nSecond Argument: Path to the directory that the backup should be resotred to\nThird Argument: the encryption key"
		return 0	
	elif [[ $# -lt 3 ]]; then
		echo "Error: Not Enough Arguments"
		return 0
	elif [[ $# -gt 3 ]]; then
		echo "Error: Arguments exceeded 3"
		return 0
	fi

	back_up_dir=$1
	back_up_store_dir=$2

	if [[ ! -d $back_up_dir ]]; then
		echo "The directory to be backed up is not a valid one"
		return 0
	elif [[ ! -d $back_up_store_dir ]]; then
		echo "The directory to store the back up inside is not valid"
		return 0
	fi
	return 1
}

backup(){
	tobe_back_up_dir=$1
	back_up_store_dir=$2
	encrypt_key=$3
	date=$(date)
	date=$(echo $date | sed s/\ /_/g)
	date=$(echo $date | sed s/:/_/g)


	new_dir="${back_up_store_dir}/${date}"
	mkdir -p $new_dir
	i=0
	for d in $tobe_back_up_dir/*
	do
		#echo "${d} is a File in my directory"
		if [[ -d $d ]]; then
	       		tar -cvzf "${d}_${date}.tgz" $d
			#echo "Encrypt Key: ${encrypt_key}"
			#echo "Target File: ${d}_${date}.tgz"
			gpg --pinentry-mode loopback --passphrase $encrypt_key --symmetric "${d}_${date}.tgz"
		        rm "${d}_${date}.tgz"	
			mv "${d}_${date}.tgz.gpg" $new_dir/
	       		#echo "${d} Directory Compressed"
		fi
	done	
	
	#Looking for files
	for d in $tobe_back_up_dir/*
	do
		if [[ ! -d $d ]]; then
	       		if [[ $i -eq 0 ]]; then
		       		tar -cvf "${tobe_back_up_dir}_files_${date}.tar" $d
				i=$((i+1))
				#echo "${d} File compressed"
			else
				tar -uvf "${tobe_back_up_dir}_files_${date}.tar" $d
				#echo "${d} File added to the compression"
			fi
		fi
	done	
	#echo "i equals to $i"
	if [[ $i -gt 0 ]]; then
		#mv "${tobe_back_up_dir}_files_${date}.tar" $new_dir/
		#echo "GnuPG Here Last of first loop"
		gzip "${tobe_back_up_dir}_files_${date}.tar"
	       	gpg --pinentry-mode loopback --passphrase $encrypt_key --symmetric "${tobe_back_up_dir}_files_${date}.tar.gz"	
	       	rm "${tobe_back_up_dir}_files_${date}.tar.gz"
		mv "${tobe_back_up_dir}_files_${date}.tar.gz.gpg" $new_dir/
		#echo "Set of Files has been compressed using gzip"
	fi

}
restore(){
	backup_dir=$1
	restore_dir=$2
	encrypt_key=$3

	new_dir="${restore_dir}/temp"
	mkdir -p $new_dir

	for d in $backup_dir/*
	do
		#echo "Directory: $d"
		#echo "Ecrypt Key: $encrypt_key"
		file="${d%.gpg}"
		gpg -d --pinentry-mode loopback --passphrase $encrypt_key --output $file --decrypt $d
		#file="${d%.gpg}"
		#echo "AUgmented file string: $file"
		mv $file $new_dir/
	done	
	
	for d in $new_dir/*
	do
		#echo "Directory: $d"
		tar zxvf $d -C $restore_dir/
	done
	rm -r $new_dir
}
