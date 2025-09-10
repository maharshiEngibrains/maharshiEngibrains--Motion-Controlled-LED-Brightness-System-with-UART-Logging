#!/bin/bash

#Key Word is always fixed for this use case. 
START_KEY="<projects>"
STOP_KEY="</projects>"
DELETE_KEY="<project>"
#Set LOGGING=false, to disable all logs from this script.
LOGGING=0
#
#Project Files
PROJ_FILE_NAME=".project"
PROJ_FILE_LIST="NULL"
#
#
# Find START_KEY and STOP_KEY. 
# Delete all the lines between START_KEY and STOP_KEY, if they contain DELETE_KEY
# Lines without DELETE_KEY are retained
# Line replacement is done inline. 
process_install()
{
FILENAME=$1
print_message "Requested Operation Started !"
while read line; do
    if [[ "$line" == *"$START_KEY"* ]]; then
        print_message "START_KEY $START_KEY found"
        while read line; do
            if [[ "$line" == *"$STOP_KEY"* ]]; then
                print_message "STOP_KEY $STOP_KEY found"
                # One the stop Key encountered, break from this loop. 
                # We don't want to continue further on same file. 
                file_processing_complete=1
                break
            else
                print_message "* Deleting a line from file in-line, if line contains DELETE_KEY**"
                sed -i /$DELETE_KEY/d $FILENAME
            fi
        done
        if [ file_processing_complete==1 ]; then
            print_message "Completed processing of $FILENAME"
            break
        fi
    else
        print_message "Read a Line; START_KEY Not found yet"
    fi
done < "$1"
print_message "Requested Operation Completed, without finding a valid STOP_KEY !"
}
#
#
print_message()
{
    if [ $LOGGING == "1" ]; then
        echo "Param #1 is $1"
    fi
}
#
#
get_files()
{
    PROJ_FILE_LIST=$(find ../../ -type f -name "$PROJ_FILE_NAME")
    #print_message "$PROJ_FILE_LIST"
    for FILENAME in $PROJ_FILE_LIST
    do
        print_message $FILENAME
    done
}
#
# Check if the script is called with any arguments. Scripts expects NO argument.
if [ $# -ne 0 ] ; then
    echo "Usage: ./prepare_build.sh"
    exit
fi
#
#
# List down all files that needs to be modified 
get_files
#
#process All Files one by one.
for FILENAME in $PROJ_FILE_LIST 
do 
    print_message "Starting processing of file: $FILENAME"
    process_install $FILENAME
    sleep 1
    sync
done
#
#
#
