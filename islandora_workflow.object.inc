<?php
/**
 * @file
 *   This file should hold all object level helper funcitons for the workflow module.
 *   I'd like there to be a class that extends the api object class.
 */

/**
 * Should return a blank option, everyone with perms associated with the indicated collection and state, object owner
 * @param array $workflow_states: The states to return the possible assignees for, if null return all
 *  (this is cheaper than working on present workflow state)
 * @param array $collections: The set of collections to return the possible assignees for
 * @param boolean $format:  Whether or not to return a string format that is usable for a select
 * @return array : $possible_assignees_by_collection[$collection][$workflow_state][user]='formated_user'
 */
function get_possible_assignees($collections, $workflow_states=NULL, $format=FALSE) {
 
 $possible_assignees_by_collection = array();
 
 
 $admins = array();
 $managers = array();
 $editors = array();
 $submitters = array();
 
 //@TODO: optimize this with some 'if '' is in workflow_states'; we don't need to know all the perms all the time
 $admins = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Administrator');
 $managers = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Manager');
 $editors = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Editor');
 $submitters = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Submitter');
 
 //This crazy set of stuff can mostly be repalced with array_replace_recursive() in php3.5+
 $collection_pids = array_unique(array_merge(array_keys($admins), array_keys($managers), array_keys($editors), array_keys($submitters)));
 foreach ($collection_pids as $collection_pid) {
  $collection = array();
  //workflow state should go here
  foreach ($workflow_states as $workflow_state) {
   
   if ($workflow_state == 'rejected' || $workflow_state == 'created') {
    if (isset($admins[$collection_pid])) {
     $collection = array_merge($collection, $admins[$collection_pid]);
    }
    if (isset($managers[$collection_pid])) {
     $collection = array_merge($collection, $managers[$collection_pid]);
    }
    if (isset($editors[$collection_pid])) {
     $collection = array_merge($collection, $editors[$collection_pid]);
    }
    if (isset($submitters[$collection_pid])) {
     $collection = array_merge($collection, $submitters[$collection_pid]);
    }
   }
   elseif ($workflow_state == 'submitted') {
    if (isset($admins[$collection_pid])) {
     $collection = array_merge($collection, $admins[$collection_pid]);
    }
    if (isset($managers[$collection_pid])) {
     $collection = array_merge($collection, $managers[$collection_pid]);
    }
    if (isset($editors[$collection_pid])) {
     $collection = array_merge($collection, $editors[$collection_pid]);
    }
   }
   elseif ($workflow_state == 'approved' || $workflow_state == 'published') {
    if (isset($admins[$collection_pid])) {
     $collection = array_merge($collection, $admins[$collection_pid]);
    }
    if (isset($managers[$collection_pid])) {
     $collection = array_merge($collection, $managers[$collection_pid]);
    }
   }

   //format to include the permission level of the user
   //these do not simply replace the existing string because the base case involves searching based on assignee name
   if ($format) {
    foreach ($collection as $user_key=>$user) {
     if(in_array($user, $admins[$collection_pid])) {
      $collection[$user] = 'Admin: ' . $user;//abbreviation used here because of select size
     }
     elseif (in_array($user, $managers[$collection_pid])) {
      $collection[$user] = 'Manager: ' . $user;
     }
     elseif(in_array($user, $editors[$collection_pid])) {
      $collection[$user] = 'Editor: ' . $user;
     }
     elseif(in_array($user, $submitters[$collection_pid])) {
      $collection[$user] = 'Submitter: ' . $user;
     }
     unset($collection[$user_key]);
    }
   }
 
   //prepend the nobody user
   array_unshift($collection, '');
   $collection = array_unique($collection);
   $possible_assignees_by_collection[$collection_pid][$workflow_state] = $collection;
   }
  }
  
 return $possible_assignees_by_collection;
}

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