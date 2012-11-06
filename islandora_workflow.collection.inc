<?php

/**
 * @file
 * This .inc file is meant to hold all helper functions regarding collections.
 */

/**
 * Get a list of all of the Islandora collections in a Fedora repository.
 * @author
 *   William Panting
 *
 * @return array
 *   $collection_list an associated array of collection pids and names
 */
function islandora_workflow_get_collections($state_tracked = NULL) {
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');
  module_load_include('inc', 'islandora_workflow');
  
  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/collection_workflow_states_query.txt';
  $query_file_handle = fopen($query_file_name, "r");
  $query_string = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  
  $query_results = ObjectHelper::performRiQuery($query_string, 'sparql');
  $collections = array();
  foreach ($query_results as $result) {
    $collection_pid = $result['collection_object'];
    $collection_pid = preg_replace('/^info:fedora\//', '', $collection_pid);
    if (empty($collection_pid)) {
      continue;
    }
        
    if ($state_tracked) {
      if ($state_tracked == 'active') {
        if ($result['state'] == 'fedora-system:def/model#Active') {
          $collections[$collection_pid] = array(
            'label' => $result['label'],
            'workflow_tracking' => 'TRUE' == $result['workflow_tracking'],
          );
        }
      }
      elseif ($state_tracked == 'inactive') {
        if ($result['state'] == 'fedora-system:def/model#Inactive') {
          $collections[$collection_pid] = array(
            'label' => $result['label'],
            'workflow_tracking' => 'TRUE' == $result['workflow_tracking'],
          );
        }
      }
      elseif ($state_tracked == 'both') {
        if ($result['state'] == 'fedora-system:def/model#Active' || $result['state'] == 'fedora-system:def/model#Inactive') {
          $collections[$collection_pid] = array(
            'label' => $result['label'],
            'workflow_tracking' => 'TRUE' == $result['workflow_tracking'],
          );
        }
      }
    }
    // Return all collections
    else {
      $collections[$collection_pid] = array(
        'label' => $result['label'],
        'workflow_tracking' => 'TRUE' == $result['workflow_tracking'],
      );
    }
  }
  // Strip out non-applicable collections via namespace.
  $collections = islandora_workflow_limit_collections_by_namespace($collections);
  return $collections;
}

/**
 * Mark a collection as being tracked by workflow via a RELS-EXT relationship.
 *
 * @param string $object_pid
 *   The object to set the relationship on
 *
 * @return object
 *   fedora http response
 */
function islandora_workflow_track_collection_in_workflow($object_pid) {
  module_load_include('inc', 'islandora_workflow');
  return (islandora_workflow_set_object_relationship($object_pid, 'is_tracked_by_workflow', 'TRUE'));
}

/**
 * Mark a collection as not being tracked by workflow via RELS-EXT.
 *
 * @param string $object_pid
 *   The object to set the relationship on
 *
 * @return object
 *   Fedora HTTP response
 */
function islandora_workflow_stop_tracking_collection_in_workflow($object_pid) {
  module_load_include('inc', 'islandora_workflow');
  return (islandora_workflow_set_object_relationship($object_pid, 'is_tracked_by_workflow', 'FALSE'));
}

/**
 * Check an object's RELS-EXT to see if the collection is tracked in workflow.
 *
 * @param string $object_pid
 *   The PID of the collection object to check on
 *
 * @return boolean
 *   TRUE if the collection is tracked in workflow, FALSE otherwise.
 */
function islandora_workflow_is_collection_workflow_tracked($object_pid) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $workflow_namespace = 'info:islandora/islandora-system:def/islandora_workflow#';

  $item = new Fedora_Item($object_pid);
  if (!$item->exists()) {
    return FALSE;
  }
  $workflow_relationships = $item->get_rdf_relationships($workflow_namespace);
  if (!empty($workflow_relationships) && !empty($workflow_relationships['is_tracked_by_workflow'])) {
    return (in_array('TRUE', $workflow_relationships['is_tracked_by_workflow']));
  }
  return FALSE;

}

/**
 * @todo
 *   remove this function when get_possible_assignees is refactored
 */
function get_possible_assignees_for_collections($collections, $workflow_states = array(), $format = FALSE) {
  module_load_include('permissions.inc', 'islandora_workflow');

  if (empty($workflow_states)) {
    // Check all workflow states.
    $workflow_states[] = 'rejected';
    $workflow_states[] = 'created';
    $workflow_states[] = 'submitted';
    $workflow_states[] = 'approved';
    $workflow_states[] = 'published';
  }
  $possible_assignees_by_collection = array();
  $admins = array();
  $managers = array();
  $editors = array();
  $submitters = array();

  /* @TODO: optimize this with some 'if '' is in workflow_states';
   * we don't need to know all the perms all the time.*/
  $admins = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Administrator');
  $managers = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Manager');
  $editors = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Editor');
  $submitters = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Submitter');
  /* This crazy set of stuff can mostly be repalced with
  array_replace_recursive() in php3.5+*/
  $collection_pids = array_unique(array_merge(array_keys($admins), array_keys($managers), array_keys($editors), array_keys($submitters)));
  foreach ($collection_pids as $collection_pid) {
    $collection = array();
    // Set the nobody user.
    $collection[''] = '';
    // Workflow state should go here.
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

      // Format to include the permission level of the user.
      /* These do not simply replace the existing string because the base case
       * involves searching based on assignee name.*/
      if ($format) {
        foreach ($collection as $user_key => $user) {
          if (in_array($user, $admins[$collection_pid])) {
            // Abbreviation used here because of select size.
            $collection[$user] = 'Admin: ' . $user;
          }
          elseif (in_array($user, $managers[$collection_pid])) {
            $collection[$user] = 'Manager: ' . $user;
          }
          elseif (in_array($user, $editors[$collection_pid])) {
            $collection[$user] = 'Editor: ' . $user;
          }
          elseif (in_array($user, $submitters[$collection_pid])) {
            $collection[$user] = 'Submitter: ' . $user;
          }
          if ($collection[$user_key] != '') {
            unset($collection[$user_key]);
          }
        }
      }

      $collection = array_unique($collection);
      $possible_assignees_by_collection[$collection_pid][$workflow_state] = $collection;
    }
  }
  return $possible_assignees_by_collection;
}

/**
 * Get a list of possible assignees (including none) for the given objects.
 *
 * @param array $objects
 *   The set of objects to return the possible assignees for
 * @param array $workflow_states
 *   The states to return the possible assignees for. If empty, return all
 *   (this is cheaper than working on present workflow state)
 * @param boolean $format
 *   Whether or not to return a string format that is usable for a <select> tag
 *
 * @return array
 *   $possible_assignees[object_pid][workflow_state]=>possible_object_assignees
 */
function get_possible_assignees($objects, $workflow_states = array(), $format = FALSE) {
  module_load_include('permissions.inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow');

  if (empty($workflow_states)) {
    // Check all workflow states.
    $workflow_states[] = 'rejected';
    $workflow_states[] = 'created';
    $workflow_states[] = 'submitted';
    $workflow_states[] = 'approved';
    $workflow_states[] = 'published';
  }
  $possible_assignees = array();

  foreach ($objects as $object_pid) {
    $possible_object_assignees = array();

    // Get the parent collection.
    $collection_pid = islandora_workflow_get_object_parent($object_pid);
    // Get the perms on the collection.
    $admins = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Administrator');
    $managers = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Manager');
    $editors = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Editor');
    $submitters = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Submitter');

    // Set the nobody user.
    $possible_object_assignees[''] = '';
    // Workflow state should go here.
    foreach ($workflow_states as $workflow_state) {
      if ($workflow_state == 'rejected' || $workflow_state == 'created') {
        if (isset($admins[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $admins[$collection_pid]);
        }
        if (isset($managers[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $managers[$collection_pid]);
        }
        if (isset($editors[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $editors[$collection_pid]);
        }
        // Add the owner to the possible assignees.
        $owner = array(islandora_workflow_get_object_creator($object_pid));
        $possible_object_assignees = array_merge($possible_object_assignees, $owner);
      }
      elseif ($workflow_state == 'submitted') {
        if (isset($admins[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $admins[$collection_pid]);
        }
        if (isset($managers[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $managers[$collection_pid]);
        }
        if (isset($editors[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $editors[$collection_pid]);
        }
      }
      elseif ($workflow_state == 'approved' || $workflow_state == 'published') {
        if (isset($admins[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $admins[$collection_pid]);
        }
        if (isset($managers[$collection_pid])) {
          $possible_object_assignees = array_merge($possible_object_assignees, $managers[$collection_pid]);
        }
      }
      // Format to include the permission level of the user.
      /* These do not simply replace the existing string because the base case
       * involves searching based on assignee name.*/
      if ($format) {
        foreach ($possible_object_assignees as $user_key => $user) {
          if (in_array($user, $admins[$collection_pid])) {
            // Abbreviation used here because of select size.
            $possible_object_assignees[$user] = 'Admin: ' . $user;
          }
          elseif (in_array($user, $managers[$collection_pid])) {
            $possible_object_assignees[$user] = 'Manager: ' . $user;
          }
          elseif (in_array($user, $editors[$collection_pid])) {
            $possible_object_assignees[$user] = 'Editor: ' . $user;
          }
          elseif (in_array($user, $submitters[$collection_pid])) {
            $possible_object_assignees[$user] = 'Submitter: ' . $user;
          }
          if ($possible_object_assignees[$user_key] != '') {
            unset($possible_object_assignees[$user_key]);
          }
        }
      }

      $possible_object_assignees = array_unique($possible_object_assignees);
      $possible_assignees[$object_pid][$workflow_state] = $possible_object_assignees;
    }
  }
  return $possible_assignees;
}
