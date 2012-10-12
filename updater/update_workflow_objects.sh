#!/bin/bash

#This bash script was used for dev/testing.
#The command should be of the form:
#python path_to_update_workflow_objects.py path_to_config_file
#All other info should be in the config file.  The dev cfg is present as an example.


echo "Running workflow object updater."

mkdir ./logs

python ./update_workflow_objects.py ./updater.cfg &> ./logs/update_workflow_objects.txt

echo "Done."