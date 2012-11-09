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
    // Return all collections.
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
 * Check an object's content model to determine whether it is a collection.
 * (and it's content model's content model)
 *
 * @param string $object_pid
 *   The Fedora PID to check for collectionism.
 *
 * @return boolean
 *   FALSE if the object or its parent is not the 'islandora:collectionCModel'
 */
function islandora_workflow_check_collection_content_model($object_pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_pid);
  $object_relations = $item->get_rdf_relationships();

  if (count($object_relations['fedora-model:hasModel']) > 0) {
    if (in_array('islandora:collectionCModel', $object_relations['fedora-model:hasModel'])) {
      return TRUE;
    }
    else {
      foreach ($object_relations['fedora-model:hasModel'] as $object_model) {
        $parent_model = new Fedora_Item($object_model);
        $parent_relations = $parent_model->get_rdf_relationships();

        if (count($parent_relations['fedora-model:hasModel']) > 0) {
          if (in_array('islandora:collectionCModel', $parent_relations['fedora-model:hasModel'])) {
            return TRUE;
          }
        }
      }
    }
  }
  return FALSE;
}

/**
 * Get a list of collections for which the current user has a given permission.
 *
 * This function uses the get_all_collections() function
 * in the case of an admin user
 * but otherwise will use the supplied permission string to search
 * the drupal database table 'islandora_workflow_permissions'
 * to compile a list of
 * collections that the user has the supplied permission to
 * This is also handy to determine if the user has any permissions at that lvl:
 * if (islandora_workflow_get_users_collections($perm)!=FALSE) {}
 * if (islandora_workflow_get_users_collections()) {}
 *
 * @return $collection_list
 *   an associated array of collection PIDs and names.
 * @return boolean
 *   FALSE if the current user doesn't have the permission for any collection.
 */
function islandora_workflow_get_users_collections($permission_level = NULL, $account = NULL, $workflow_tracked = NULL) {
  module_load_include('permissions.inc', 'islandora_workflow');

  global $user;

  if (is_null($permission_level)) {
    $permission_level = 'islandora_workflow_Manager';
  }

  if (empty($account)) {
    $account = drupal_clone($user);
  }

  $permitted_collections = array();
  $collection_list = array();
  if (user_access('islandora_workflow_Administrator', $account) || user_access('administer', $account)) {
    // Admin show all.
    $collection_list = islandora_workflow_get_collections();
  }
  elseif (user_access($permission_level, $account) || $permission_level == 'all') {
    // Manager show permitted.
    $collection_list = islandora_workflow_get_collections();
    // Figure out owned collections.
    // Role perms.
    $users_role_collection_permissions = islandora_workflow_get_users_role_permissions($account);

    foreach ($users_role_collection_permissions as $role_id => $collections) {
      foreach ($collections as $collection_id => $permission) {
        // All available permissions.
        if ($permission_level == 'all') {
          $permitted_collections[] = $collection_id;
        }
        elseif ($permission == $permission_level) {
          $permitted_collections[] = $collection_id;
        }
      }
    }
    // User perms.
    $users_user_collection_permissions = islandora_workflow_get_users_user_permissions($account);

    foreach ($users_user_collection_permissions as $role_id => $collections) {
      foreach ($collections as $collection_id => $permission) {
        if ($permission_level == 'all') {
          // All available permissions.
          $permitted_collections[] = $collection_id;
        }
        elseif ($permission == $permission_level) {
          $permitted_collections[] = $collection_id;
        }
      }
    }
    // Only send back owned collection pids and names.
    foreach ($collection_list as $collection_pid => $collection_information) {
      if (!in_array($collection_pid, $permitted_collections)) {
        unset($collection_list[$collection_pid]);
      }
    }
  }
  else {
    return array();
  }

  if ($workflow_tracked) {
    $tracked_collections = array();
    foreach ($collection_list as $key => $value) {
      if ($value['workflow_tracking']) {
        $tracked_collections[$key] = $value;
      }
    }
    $collection_list = $tracked_collections;
  }

  return $collection_list;
}

/**
 * Get all members of a collection.
 *
 * This function gets all the members (with a workflow status)
 * of a collection through the relationship of 'isMemberOf'
 * and 'isMemberOfCollection' the two relationships need to be
 * checked because there is no
 * Fedora enforced standard.
 * @author
 *   William Panting
 *
 * @param array $collection_id
 *   The PID of the collection.
 *
 * @return array
 *   $member_list_full The array containing all the pids and relevent
 *   info from the RI.
 * @return boolean
 *   FALSE if there are no members of the collection.
 */
function islandora_workflow_get_all_members_of_collection($collection_id) {
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');

  // Get info for query.
  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/sparql/member_query.sparql';
  $query_file_handle = fopen($query_file_name, "r");
  $query = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);

  // Inject collection value into the query.
  $pattern = 'BOUND(?collection_object)';
  $replacement = "?collection_object = <info:fedora/$collection_id>";

  $query = str_replace($pattern, $replacement, $query);
  $limit = -1;
  $query_results = ObjectHelper::performRiQuery($query, 'sparql', $limit, $offset = 0);
  $processed_results = array();
  foreach ($query_results as $result) {
    $index = preg_replace('/^info:fedora\//', '', $result['member_object']);
    $result['islandora_workflow_modified'] = $result['timestamp'];
    unset($result['timestamp']);
    $processed_results[$index] = $result;
  }

  return $processed_results;
}

/**
 * Filter a list of collections by a set of namespaces.
 *
 * This function will reduce the results on a collection search down to those
 * applicable to this install of Islandora.
 * @author
 *   Paul Pound
 * @author
 *   William Panting
 *
 * @param array $existing_collections
 *   The list of collections before modification
 * @param array $pid_namespaces
 *   The list of namespaces that are applicable to this Islandora install
 *
 * @return array
 *   $collections  The collections that exist in the indicated namespaces
 */
function islandora_workflow_limit_collections_by_namespace($existing_collections, $pid_namespaces = NULL) {
  /* If no namespace list supplied get it
   * from fedora_repository module's variables.*/
  if (variable_get('fedora_namespace_restriction_enforced', TRUE)) {
    if ($pid_namespaces == NULL) {
      $pid_namespaces = array();
      foreach (explode(' ', trim(variable_get('fedora_pids_allowed', 'default: demo: changeme: Islandora: ilives: '))) as $namespace) {
        $pid_namespaces[$namespace] = $namespace;
      }
    }

    $collections = array();
    foreach ($existing_collections as $collection => $value) {
      foreach ($pid_namespaces as $key => $namespace) {
        if (strpos($collection, $namespace) === 0) {
          $collections[$collection] = $value;
        }
      }
    }
    return $collections;
  }
  else {
    return $existing_collections;
  }
}
