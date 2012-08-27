<?php
/**
 * @file
 * This file should hold object level helper functions for the workflow module.
 *   I'd like there to be a class that extends the api object class.
 */

/**
 * Remove the assignee of one or more objects.
 *
 * This function will nullify the assignee if the passed in assignee
 * will not be able to modify the object if it is in the passed in state
 *
 * @param string $fedora_pid
 *   The Fedora object to possibly remove the assignee from.
 * @param string $collection
 *   The collection the object belongs to... it's nice to get
 *   this(performance), but maybe should be optional
 * @param string $workflow_state
 *   The state to normalize against
 * @param string $assignee
 *   The current assignee of the object
 */
function normalize_assignee($fedora_pid, $collection, $workflow_state, $assignee) {
  $possible_assignee_list = get_possible_assignees($collection, $workflow_state);

  if (in_array($assignee, $possible_assignee_list)) {
    // Normalize.
    $i = 1;
  }
}
