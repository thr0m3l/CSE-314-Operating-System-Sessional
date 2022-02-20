#!/bin/bash

declare -a extensions
declare -a ignore_extensions
input_file=""
work_dir="${PWD}"

function manual(){
    echo "Usage: ./1705069.sh [directory](optional)[file_name](txt)
    directory: path of the directory of the folder to be worked on.
    file_name: path of the text file containing the list of the extensions that are to be ignored.

    example: ./1705069.sh /home/romel/ /home/romel/ignore.txt"
}

function handle_arg(){
    echo $#
    
    if [ $# -eq 0 ]; then
        manual
        exit 1
    fi
    
    if [ $# -eq 1 ]; then
        input_file=$1
        work_dir=$HOME
    fi
    
    if [ $# -eq 2 ]; then
        input_file=$2
        
        if [ $1 == "." ]; then
            work_dir=$PWD
        else
            work_dir=$1
        fi
    fi
    
    echo "work_dir=${work_dir}"
    echo "input_file=${input_file}"
}


function getextension(){
    extensions=($(find "${work_dir}" -type f -name '*.*' | sed 's|.*\.||' | sort -u))
}

function readfile(){
    #Reads from the files, retrieves all extensions that are to be ignored
    #and then deletes them from the original extensions array
    
    echo "Reading from ${input_file}"
    IFS=$'\n' read -d '' -r ignore_extensions < "${input_file}"
    
    for ig in ${ignore_extensions[@]}; do
        extensions=( "${extensions[@]/$ig}" )
    done
}

function makedirectories(){
    for ext in "${extensions[@]}"
    do
        if [ ! -d "${work_dir}/output/${ext}" ]; then
            mkdir -p "${work_dir}/output/${ext}"
        fi
    done
    if [ ! -d "${work_dir}/output/others" ]; then
        mkdir -p "${work_dir}/output/others"
    fi
}

function copy(){
    #Copies the files to their respective directories according to their extensions
    #Generates description and csv
    
    cd $work_dir
    echo "file_type,no_of_files" >> "./output/output.csv"
    
    for ext in "${extensions[@]}"
    do
        
        filename="./output/${ext}/desc_${ext}.txt"
        total_files=$(find . -name "*.${ext}" | wc -l)
        if [ -n "${ext}" ]; then
            echo "${ext},${total_files}" >> "./output/output.csv"
            find . -name "*.${ext}" | tee $filename
        fi
        
        find "${work_dir}" -name "*.${ext}" -exec cp -vunif '{}' "${work_dir}/output/${ext}/" ";"
    done
    
    #others
    find . -type f ! -name "*.*" -exec cp -vunif '{}' "${work_dir}/output/others/" ";"
    find . -type f ! -name "*.*" | tee "./output/others/desc_others.txt"
    total_files=$(find . -type f ! -name "*.*" | wc -l)
    echo "others,${total_files}" >> "./output/output.csv"
}



# rm -rf "./$work_dir/output"
handle_arg $@
getextension
readfile
makedirectories
copy