source ./backup_restore_lib.sh
validate_restore_params $1 $2 $3
i=$?
if [[ $i -eq 0 ]]; then
	exit 1
	#echo "ERROR OCURRED. NOT A GOOD SIGN"
fi

restore $1 $2 $3
