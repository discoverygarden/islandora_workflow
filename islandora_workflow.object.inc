<?php
/**
 * @file
 *   This file should hold all object level helper funcitons for the workflow module.
 *   I'd like there to be a class that extends the api object class.
 */

/**
 * This function will nullify the assignee if the passed in assignee
 * will not be able to modify the object if it is in the passed in state
 * @param $fedora_pid:
 * @param $collection:
 * @param $workflow_state: The state to normalize against
 * @param $assignee:  The current assignee of the object
 */
function normalize_assignee($fedora_pid, $collection, $workflow_state, $assignee) {
 $possible_assignee_list = get_possible_assignees($collection, $workflow_state);
 
 if(in_array($assignee, $possible_assignee_list)) {
  //normalize
  $i=1;
 }
}