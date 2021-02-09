. ~/ex1_bash_scripting/backup_restore_lib.sh
validate_backup_params $1 $2 $3
i=$?
if [[ $i -eq 0 ]]; then
	exit 1
	#echo "Error Occured. NOT A GOOD SIGN"
fi
backup $1 $2 $3

