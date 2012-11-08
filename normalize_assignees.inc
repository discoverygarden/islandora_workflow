<?php
/**
 * @file
 *   Handles assignment of Islandora workflow objects.
 */

/**
 * Removes the named user as the assignee for a Fedora object.
 *
 * @staticvar FedoraAPIM $apim
 *   the FedoraAPIM object
 *
 * @param string $object_pid
 *   The PID of the Fedora object
 * @param string $assignee
 *   The Drupal username of the assignee.
 *
 * @return boolean
 *   TRUE if any relationships were purged, FALSE otherwise.
 */
function islandora_workflow_remove_assignee($object_pid, $assignee = NULL) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $islandora_workflow_namespace = 'info:islandora/islandora-system:def/islandora_workflow#';
  $predicate = 'assignee';

  $item = new Fedora_Item($object_pid);
  if (!$item->exists()) {
    return FALSE;
  }

  $item->purge_relationships($predicate, $assignee, $islandora_workflow_namespace, RELS_TYPE_PLAIN_LITERAL);
  drupal_set_message("$assignee has been removed as the assignee to $object_pid.");
}

/**
 * Check that an object's assignee still has the right permissions
 * for the indeicated workflow state. If not, remove the assignee.
 *
 * @param string $object_pid
 *   the PID of the object.
 * @param string $state
 *   The object's new workflow state. Will default to the current state.
 */
function islandora_workflow_normalize_assignee($object_pid, $state = NULL) {
  module_load_include('inc', 'islandora_workflow');
  module_load_include('collection.inc', 'islandora_workflow');
  if (is_null($state)) {
    $state = islandora_workflow_get_object_workflow_state($object_pid);
  }
  $assignee_name = islandora_workflow_get_object_assignee($object_pid);

  if (!$assignee_name) {
    return;
  }
  $possible_assignees = islandora_workflow_get_possible_assignees(array($object_pid), $workflow_states = array($state), $format = FALSE);
  if (!in_array($assignee_name, $possible_assignees[$object_pid][$state])) {
      islandora_workflow_remove_assignee($object_pid, $assignee_name);
  }
}

/**
 * Check a collection's members for invalid assignees and remove them.
 *
 * @param string $collection_pid
 *   The PID of the parent collection.
 */
function islandora_workflow_normalize_members_assignees($collection_pid) {
  $possible_assignees = islandora_workflow_get_possible_assignees_for_collections(array($collection_pid));
  $members = islandora_workflow_assignment_by_collection($collection_pid);

  foreach ($members as $member) {
    if (empty($member['assignee'])) {
      continue;
    }
    // Check collection level perms if they will cover things.
    if ($member['state'] != 'rejected' && $member['state'] != 'created') {
      if (!in_array($member['assignee'], $possible_assignees[$collection_pid][$member['state']])) {
        islandora_workflow_remove_assignee($member['pid'], $member['assignee']);
      }
    }
    // If the state is created/rejected check the object's possible assignees.
    else {
      $member_possible_assignees = islandora_workflow_get_possible_assignees(array($object), $workflow_states = array(), $format = FALSE);
      if (!in_array($member['assignee'], $member_possible_assignees[$member['pid']][$member['state']])) {
        islandora_workflow_remove_assignee($member['pid'], $member['assignee']);
      }
    }
  }
}

/**
 *  Get a list of possible assignees (including none) for the given collections.
 *
 * @param array $collections
 *   The set of collections to return the possible assignees for
 * @param array $workflow_states
 *   The states to return the possible assignees for. If empty, return all
 *   (this is cheaper than working on present workflow state)
 * @param boolean $format
 *   Whether or not to return a string format that is usable for a <select> tag
 *
 * @return array
 *   $assignees[collection_pid][workflow_state]=>possible_object_assignees
 */
function islandora_workflow_get_possible_assignees_for_collections($collections, $workflow_states = array(), $format = FALSE) {
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

  /* @TODO: optimize this with some 'if '' is in workflow_states';
   * we don't need to know all the perms all the time.*/
  $admins = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Administrator');
  $managers = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Manager');
  $editors = islandora_workflow_get_all_users_with_collection_permissions($collections, 'islandora_workflow_Editor');

  foreach ($collections as $collection_pid) {
    // Workflow state should go here.
    foreach ($workflow_states as $workflow_state) {
      $possible_collection_assignees = array();
      // Set the nobody user.
      $possible_collection_assignees[] = '';
      if ($workflow_state == 'rejected' || $workflow_state == 'created' || $workflow_state == 'submitted') {
        if (isset($admins[$collection_pid])) {
          $possible_collection_assignees = array_merge($possible_collection_assignees, $admins[$collection_pid]);
        }
        if (isset($managers[$collection_pid])) {
          $possible_collection_assignees = array_merge($possible_collection_assignees, $managers[$collection_pid]);
        }
        if (isset($editors[$collection_pid])) {
          $possible_collection_assignees = array_merge($possible_collection_assignees, $editors[$collection_pid]);
        }
      }
      elseif ($workflow_state == 'approved' || $workflow_state == 'published') {
        if (isset($admins[$collection_pid])) {
          $possible_collection_assignees = array_merge($possible_collection_assignees, $admins[$collection_pid]);
        }
        if (isset($managers[$collection_pid])) {
          $possible_collection_assignees = array_merge($possible_collection_assignees, $managers[$collection_pid]);
        }
      }

      // Format to include the permission level of the user.
      /* These do not simply replace the existing string because the base case
       * involves searching based on assignee name.*/
      if ($format) {
        foreach ($possible_collection_assignees as $user_key => $user) {
          if (in_array($user, $admins[$collection_pid])) {
            // Abbreviation used here because of select size.
            $possible_collection_assignees[$user] = 'Admin: ' . $user;
          }
          elseif (in_array($user, $managers[$collection_pid])) {
            $possible_collection_assignees[$user] = 'Manager: ' . $user;
          }
          elseif (in_array($user, $editors[$collection_pid])) {
            $possible_collection_assignees[$user] = 'Editor: ' . $user;
          }
          elseif (in_array($user, $submitters[$collection_pid])) {
            $possible_collection_assignees[$user] = 'Submitter: ' . $user;
          }
          // Kill the unformated elements.
          if ($possible_collection_assignees[$user_key] != '') {
            unset($possible_collection_assignees[$user_key]);
          }
        }
      }
      $possible_collection_assignees = array_unique($possible_collection_assignees);
      $possible_assignees_by_collection[$collection_pid][$workflow_state] = $possible_collection_assignees;
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
function islandora_workflow_get_possible_assignees($objects, $workflow_states = array(), $format = FALSE) {
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

    // Get the parent collection.
    $collection_pid = islandora_workflow_get_object_parent($object_pid);
    // Get the perms on the collection.
    $admins = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Administrator');
    $managers = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Manager');
    $editors = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Editor');
    $submitters = islandora_workflow_get_all_users_with_collection_permissions(array($collection_pid), 'islandora_workflow_Submitter');

    // Workflow state should go here.
    foreach ($workflow_states as $workflow_state) {

      $possible_object_assignees = array();
      // Set the nobody user.
      $possible_object_assignees[] = '';
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
          // Kill the unformated elements.
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

/**
 * Return all objects assigned to a given user, along with their workflow state.
 *
 * @param object $account
 *   a user object.
 *
 * @return array
 *   an array of results.
 */
function islandora_workflow_assignment_by_user($account, $limit = -1, $offset = 0) {

  module_load_include('inc', 'fedora_repository', 'ObjectHelper');

  global $user;

  if (empty($account)) {
    $account = drupal_clone($user);
  }

  // Get the stored query.
  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/sparql/member_query.sparql';
  $query_file_handle = fopen($query_file_name, "r");
  $query_string = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);

  // We don't want results where ?assignee is not bound.
  $query = str_replace('!BOUND(?assignee) || ', '', $query_string);
  $query = str_replace("REGEX(STR(?assignee),'.?')", "STR(?assignee) = '" . $account->name . "'", $query);
  $query_results = ObjectHelper::performRiQuery($query, 'sparql', $limit, $offset);

  $objects = array();
  foreach ($query_results as $result) {
    $index = preg_replace('/^info:fedora\//', '', $result['member_object']);
    $result['islandora_workflow_modified'] = $result['timestamp'];
    unset($result['timestamp']);
    $objects[$index] = $result;
  }
  return $objects;

}

/**
 * Return all assignees for members of a given collection.
 *
 * @param string $collection_pid
 *   the PID of the collection.
 *
 * @return array
 *   an array of results.
 */
function islandora_workflow_assignment_by_collection($collection_pid) {
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');

  // Get the stored query.
  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/sparql/assignee_query.sparql';
  $query_file_handle = fopen($query_file_name, "r");
  $query = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);

  $query = str_replace('?collection', "<info:fedora/$collection_pid>", $query);
  $limit = -1;
  $query_results = ObjectHelper::performRiQuery($query, 'sparql', $limit, $offset = 0);

  return $query_results;
}

/**
 * Remove an about-to-be-deleted user from their assignments.
 *
 * Returns an array of batch operations.
 *
 * @param user $account
 *   The original user account.
 *
 * @return array
 *   An array of batch operations.
 */
function islandora_workflow_user_delete_assignees_batch($account = NULL) {

  $operations = array();
  if (is_null($account)) {
    global $user;
    $account = drupal_clone($user);
  }

  $assigned_objects = islandora_workflow_assignment_by_user($account);
  foreach ($assigned_objects as $object_PID => $object_info) {
    $operations[] = array(
      'islandora_workflow_remove_assignee_batch',
      array($object_PID, $account->name),
    );
  }
  return $operations;
}

/**
 * Check the validity of a user's assignments after their roles have changed.
 * Needs to run after the workflow perms tables are updated.
 *
 * @param object $edit
 *   The edit mad to the user account.
 * @param user $account
 *   The original user account.
 * @param string $category
 *   The category of edit being made.
 *
 * @return array
 *   An array of batch operations
 */
function islandora_workflow_user_edit_assignees_batch($edit, $account, $category = NULL) {
  // Run a batch operation if the user's roles have changed.
  $operations = array();
  if (empty($edit['roles'])) {
    return $operations;
  }
  $roles_diff = array_diff($account->roles, $edit['roles']);
  if (empty($roles_diff)) {
    return $operations;
  }
  $assigned_objects = islandora_workflow_assignment_by_user($account);
  // Find each object that is assigned to the user and normalize.
  // XXX: could be optimized to only effected objects?
  foreach ($assigned_objects as $object_PID => $object_info) {
    $operations[] = array(
      'islandora_workflow_normalize_assignee_batch',
      array($object_PID),
    );
  }
  return $operations;
}
