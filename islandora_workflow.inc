<?php
/**
 * @file
 * This is a collection of helper functions for the islandora_workflow module.
 * @author
 *   William Panting
 * @todo: refactor out object and collection level funcs to their own .incs
 */

/**
 * Get a list of collections for which the current user has a given permission.
 *
 * This function uses the get_all_collections() function
 * in the case of an admin user
 * but otherwise will use the supplied permission string to search
 * the drupal database table 'islandora_workflow_permissions'
 * to compile a list of
 * collections that the user has the supplied permission to
 * This is also handy to determin if the user has any permissions at that lvl:
 * if (islandora_workflow_get_users_collections($perm)!=FALSE) {}
 * if (islandora_workflow_get_users_collections()) {}
 *
 * @param string $permission_level
 *   the permission level we are looking for if it is the string
 *   'all' it will get all collections the user has any access to
 *
 * @return $collection_list
 *   an associated array of collection PIDs and names.
 * @return boolean
 *   FALSE if the current user doesn't have the permission for any collection.
 */
function islandora_workflow_get_users_collections($permission_level = 'islandora_workflow_Manager', $account = NULL) {

  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.collection');
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');

  global $user;

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
  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/member_query.txt';
  $query_file_handle = fopen($query_file_name, "r");
  $query = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);

  // Inject collection value into the query.
  $pattern = '/(REGEX.+collection_object.+?\')(\.)(\')/';
  $replacement = '\1' . $collection_id . '\3';

  $query = preg_replace($pattern, $replacement, $query);

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
   * from fedora_repository module's varaiables.*/
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

/**
 * Add a workflow tracking datastream to a Fedora object.
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 *
 * @return string
 *   $result the return mesage from fedora
 */
function islandora_workflow_init_workflow($object_id) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }

  // Attach default workflow.
  $default_xml_file_name = drupal_get_path('module', 'islandora_workflow') . '/start_workflow.xml';

  $item->add_datastream_from_file(
      $default_xml_file_name,
      $dsid = 'islandora_workflow',
      $ds_label = NULL,
      $mimetype = 'text/xml',
      $control_group = 'X',
      $log_message = 'Adding islandora_workflow datastream(editorial workflow tracking).'
  );

  // Set state.
  $result = islandora_workflow_set_object_workflow_state($object_id, 'created');

  return $result;
}

/**
 * Move a Fedora object to the 'Rejected' workflow state.
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 *
 * @return string
 *   the return mesage from Fedora
 * @return boolean
 *   FALSE if the rejection was not attempted
 */
function islandora_workflow_reject_object($object_id) {
  module_load_include('permissions.inc', 'islandora_workflow');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $label = islandora_workflow_get_object_label($object_id);

  // Permission check.
  $tmp = islandora_workflow_user_object_permission_check($object_id);
  if ($tmp != 'islandora_workflow_Editor' && $tmp != 'islandora_workflow_Manager') {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be rejected, you do not have the necessary permissions.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
    return FALSE;
  }

  $workflow_state = islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state != 'created' && $workflow_state != 'rejected') {
    // If the state is published and XACML is enabled you might not be able to.
    // Reject due to XACML.
    global $user;
    if ($workflow_state == 'published' && module_exists('islandora_xacml_api') && $user->uid != 1) {
      module_load_include('inc', 'islandora_xacml_api', 'Xacml');
      $xacml = Xacml::constructFromPid($object_id);
      if (!$xacml->managementRule->hasPermission($user->name, $user->roles)) {
        drupal_set_message(t('The object <em>%label</em> (%pid) could not be rejected, you do not have XACML access to the object.', array(
          '%label' => $label,
          '%pid' => $object_id,
        )));
        return FALSE;
      }
    }

    // Send out email to creator.
    $user_name = islandora_workflow_get_object_creator($object_id);
    if ($user_name) {
      $object = new Fedora_Item($object_id);
      rules_invoke_event('islandora_workflow_rules_reject_object', $object);
    }
    else {
      drupal_set_message(t('No email sent for the rejection of <em>%label</em> (%pid)', array(
        '%label' => $label,
        '%pid' => $object_id,
      )));
    }
    return (islandora_workflow_set_object_workflow_state($object_id, 'rejected'));
  }
  else {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be rejected, it was not in an appropriate workflow state.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
    return FALSE;
  }
}

/**
 * Move a Fedora object to the 'Approved' workflow state.
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 *
 * @return string
 *   the return mesage from Fedora
 * @return boolean
 *   FALSE if the approval was not attempted
 */
function islandora_workflow_approve_object($object_id) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $label = islandora_workflow_get_object_label($object_id);

  // Permission check.
  $tmp = islandora_workflow_user_object_permission_check($object_id);
  if ($tmp != 'islandora_workflow_Editor' && $tmp != 'islandora_workflow_Manager') {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be approved, you do not have the necessary permissions.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
    return FALSE;
  }

  $workflow_state = islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state != 'published' && $workflow_state != 'approved') {

    $object = new Fedora_Item($object_id);
    rules_invoke_event('islandora_workflow_rules_approve_object', $object);
    return (islandora_workflow_set_object_workflow_state($object_id, 'approved'));
  }
  else {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be approved, it was not in an appropriate workflow state.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
  }
  return FALSE;
}

/**
 * Move a Fedora object to the 'Published' workflow state.
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 *
 * @return string
 *   the return mesage from Fedora [on changing state to 'A']
 * @return boolean
 *   FALSE if the publication was not attempted
 */
function islandora_workflow_publish_object($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');

  $label = islandora_workflow_get_object_label($object_id);

  // Permission check.
  if (islandora_workflow_user_object_permission_check($object_id) != 'islandora_workflow_Manager') {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be published, you do not have the necessary permissions.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
    return FALSE;
  }

  if (islandora_workflow_get_object_workflow_state($object_id) != 'published') {
    islandora_workflow_set_object_workflow_state($object_id, 'published');
    // Make the object viewable.
    $label = islandora_workflow_get_object_label($object_id);
    $owner = islandora_workflow_get_object_creator($object_id);
    $item = new Fedora_Item($object_id);
    $item->modify_object($label, 'A', $owner);
  }
  else {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be published, it was not in an appropriate workflow state.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
    return FALSE;
  }
}

/**
 * Move a Fedora object to the 'Submitted' workflow state.
 *
 * [wrapper function]
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 *
 * @return string
 *   the return mesage from Fedora
 * @return boolean
 *   FALSE if the submission was not attempted
 */
function islandora_workflow_submit_object($object_id) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $label = islandora_workflow_get_object_label($object_id);

  // Permission check.
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be submitted, you do not have the necessary permissions.', array('%pid' => $object_id)));
    return FALSE;
  }

  $workflow_state = islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state == 'created' || $workflow_state == 'rejected') {

    // Send email to all editors.
    $object = new Fedora_Item($object_id);
    rules_invoke_event('islandora_workflow_rules_submit_object', $object);
    return (islandora_workflow_set_object_workflow_state($object_id, 'submitted'));
  }
  else {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be submitted, it was not in an appropriate workflow state.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
  }
  return FALSE;
}

/**
 * Assign a Fedora object to the current user.
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 *
 * @return string
 *   the return mesage from Fedora
 */
function islandora_workflow_claim_object($object_id) {
  global $user;
  $label = islandora_workflow_get_object_label($object_id);
  $present_assignee = islandora_workflow_get_object_assignee($object_id);
  if ($present_assignee == $user->name) {
    drupal_set_message(t('The object <em>%label</em> (%pid) is already assigned to you.', array(
      '%pid' => $object_id,
      '%label' => $label,
    )));
    return FALSE;
  }
  return (islandora_workflow_assign_object($object_id, $user->name));
}

/**
 * Assign a Fedora object to a named user.
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 * @param string $user_name
 *   the name of the Drupal user to assign the object to
 *
 * @return string
 *   $result the return mesage from Fedora
 */
function islandora_workflow_assign_object($object_id, $user_name) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  global $user;

  $item = new Fedora_Item($object_id);
  $label = $item->objectProfile->objLabel;

  $relationships = $item->get_rdf_relationships();
  if (isset($relationships['hasModel'])) {
    if (in_array('islandora:collectionCModel', $relationships['hasModel'])) {
      drupal_set_message(t('The object <em>%label</em> (%pid) is a collection and cannot be assigned.', array(
        '%label' => $label,
        '%pid' => $object_id,
      )));
      return FALSE;
    }
  }

  $present_assignee = islandora_workflow_get_object_assignee($object_id);
  if ($present_assignee == $user_name) {
    if ($user_name == $user->name) {
      drupal_set_message(t('The object <em>%label</em> (%pid) is already assigned to you.', array(
        '%label' => $label,
        '%pid' => $object_id,
      )));
    }
    else {
      drupal_set_message(t('The object <em>%label</em> (%pid) is already assigned to <em>%username</em>.', array(
        '%label' => $label,
        '%pid' => $object_id,
        '%username' => $user_name,
      )));
    }
    return FALSE;
  }

  // Permission check.
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be assigned, you do not have the necessary permissions.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
    return FALSE;
  }
  rules_invoke_event('islandora_workflow_rules_assign_object', $item);
  // Assign object.
  return (islandora_workflow_set_object_relationship($object_id, 'assignee', $user_name));
}

/**
 * Set the workflow state for an object.
 *
 * uses RELS-EXT.
 *
 * @param string $object_id
 *   the Fedora PID of the object to check the status of
 * @param string $state
 *   The workflow state
 *
 * @return string
 *   $result the return mesage from Fedora
 */
function islandora_workflow_set_object_workflow_state($object_id, $state) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  global $user;

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }

  // Get the collection of this object.
  $collection = islandora_workflow_get_object_parent($object_id);

  $workflow_parser = new DOMDocument();

  // Make sure there is an appropriate XACML datastream if there should be.
  if (variable_get('islandora_workflow_enforce_security', 0) && module_exists('islandora_xacml_api') && $state != 'published') {
    module_load_include('inc', 'islandora_xacml_api', 'Xacml');
    module_load_include('permissions.inc', 'islandora_workflow');

    $xacml = new Xacml();

    // Get users and roles.
    $creator = islandora_workflow_get_object_creator($object_id);

    $perms = islandora_workflow_get_permission_state($collection, $state);
    $perms['users'][] = $creator;

    $xacml->managementRule->addRole($perms['roles']);
    $xacml->viewingRule->addRole($perms['roles']);
    $xacml->managementRule->addUser($perms['users']);
    $xacml->viewingRule->addUser($perms['users']);

    if (isset($item->datastreams['POLICY'])) {
      $item->modify_datastream(
          $xacml->getXmlString(),
          $ds_id = 'POLICY',
          $ds_label = NULL,
          $mimetype = 'text/xml'
      );
    }
    else {
      $item->add_datastream_from_string(
          $xacml->getXmlString(),
          $ds_id = 'POLICY',
          $ds_label = NULL,
          $mimetype = 'text/xml',
          $control_group = 'X'
      );
    }
  }
  else {
    try {
      /* XACML is disabled. delete the xacml datastream if
       * it exists before it starts causing problems.*/
      if (isset($item->datastreams['POLICY'])) {
        $item->purge_datastream('POLICY');
      }
      $collection_item = new Fedora_Item($collection);
      $collection_datastreams = $collection_item->datastreams;

      if (isset($collection_item->datastreams['CHILD_SECURITY'])) {
        $child_security = $collection_item->get_datastream_dissemination('CHILD_SECURITY');
        $item->add_datastream_from_string(
            $child_security,
            $ds_id = 'POLICY',
            $ds_label = 'security policy',
            $mimetype = 'text/xml',
            $control_group = 'X',
            $log_message = 'Adding policy datastream.'
        );
      }
    }
    catch (Exception $e) {
      if ($e->http->code == 401) {
        // It has a policy but we can't delete it.
        drupal_set_message(t('This object has a policy datastream preventing it from being modified.'), 'error');
        return FALSE;
      }
      else {
        throw $e;
      }
    }
  }

  // Update last modified user/time.
  if (isset($item->datastreams['islandora_workflow'])) {
    // Build xml from DS.
    $workflow_string = $item->get_datastream_dissemination('islandora_workflow');
    $workflow_parser->loadXML($workflow_string);
    // Get elements to populate dynamicaly.
    $last_modified_by_array = $workflow_parser->getElementsByTagName('last_modified_by');
    $when_last_modified_array = $workflow_parser->getElementsByTagName('when_last_modified');
    // Workflow DS.
    foreach ($last_modified_by_array as $last_modified_by_element) {
      // Create text node.
      $last_modified_by = $workflow_parser->createElement('last_modified_by');
      $last_modified_by->appendChild($workflow_parser->createTextNode($user->name));
      // Insert text node.
      $last_modified_by_element->parentNode->replaceChild($last_modified_by, $last_modified_by_element);
    }
    foreach ($when_last_modified_array as $when_last_modified_element) {
      // Create text node.
      $when_last_modified = $workflow_parser->createElement('when_last_modified');
      $when_last_modified->appendChild($workflow_parser->createTextNode(date('c')));
      // Insert text node.
      $when_last_modified_element->parentNode->replaceChild($when_last_modified, $when_last_modified_element);
    }
    $item->modify_datastream(
        $workflow_parser->saveXML(),
        $ds_id = 'islandora_workflow',
        $ds_label = NULL,
        $mimetype = 'text/xml'
    );
  }
  else {
    // Build xml from default if there is no DS present.
    $default_xml_file_name = drupal_get_path('module', 'islandora_workflow') . '/start_workflow.xml';
    $workflow_parser->load($default_xml_file_name);
    // Get elements to populate dynamicaly.
    $last_modified_by_array = $workflow_parser->getElementsByTagName('last_modified_by');
    $when_last_modified_array = $workflow_parser->getElementsByTagName('when_last_modified');
    // Workflow DS.
    foreach ($last_modified_by_array as $last_modified_by_element) {
      // Create text node.
      $last_modified_by = $workflow_parser->createTextNode($user->name);
      // Insert text node.
      $last_modified_by_element->appendChild($last_modified_by);
    }
    foreach ($when_last_modified_array as $when_last_modified_element) {
      // Create text node.
      // The function date('c') is supposed to give back an xml friendly string.
      $when_last_modified = $workflow_parser->createTextNode(date('c'));

      // Insert text node.
      $when_last_modified_element->appendChild($when_last_modified);
    }
    $item->add_datastream_from_string(
        $workflow_parser->saveXML(),
        $ds_id = 'islandora_workflow',
        $ds_label = 'workflow',
        $mimetype = 'text/xml',
        $control_group = 'X',
        $log_message = 'Adding islandora_workflow stream (workflow tracking).'
    );
  }

  // Change the rels-int.
  islandora_workflow_set_object_relationship($object_id, 'has_workflow_timestamp', date('c'), 'islandora_workflow');

  /* Make sure that the object's fedora
   * state is going to be set to I if the object is still in workflow.*/
  if ($state != 'published' && islandora_workflow_get_object_fedora_state($object_id) != 'I') {
    $label = islandora_workflow_get_object_label($object_id);
    $owner = islandora_workflow_get_object_creator($object_id);
    $item->modify_object($label, 'I', $owner);
  }
  module_invoke_all('workflow_state_change', $object_id, $state);
  return (islandora_workflow_set_object_relationship($object_id, 'state', $state));
}

/**
 * Get the label for a Fedora object with a given PID.
 *
 * @param string $object_id
 *   The object to get the label of
 *
 * @return string $label
 *   The label of the object specified.
 *
 * @return boolean
 *   FALSE if there is an exception in geting the label
 */
function islandora_workflow_get_object_label($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  if (!isset($item->objectProfile->objLabel)) {
    return '';
  }
  return $item->objectProfile->objLabel;
}

/**
 * Set the Fedora state of an object to Deleted.
 *
 * This function does not purge an object.
 *
 * @param string $object_id
 *   the Fedora PID of the object to modify
 *
 * @return string
 *   $result the return mesage from Fedora
 * @return boolean
 *   FALSE if the state of the object could not be changed by the current user.
 */
function islandora_workflow_delete_object($object_id) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $label = islandora_workflow_get_object_label($object_id);

  // Permission check.
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object <em>%label</em> (%pid) could not be deleted, you do not have the necessary permissions.', array(
      '%label' => $label,
      '%pid' => $object_id,
    )));
    return FALSE;
  }

  $item = new Fedora_Item($object_id);
  $owner = islandora_workflow_get_object_creator($object_id);

  islandora_workflow_set_object_workflow_state($object_id, 'deleted');
  $response = $item->modify_object($label, 'D', $owner);
  if ($response) {
    rules_invoke_event('islandora_workflow_rules_delete_object', $item);
    return $response;
  }
  else {
    return FALSE;
  }
}

/**
 * Get the name of the creator of an object.
 *
 * Note: The name returned may not necessarily be that of a Drupal user.
 * uses object properties
 *
 * @param string $object_id
 *   the Fedora PID of the object to check the status of
 *
 * @return string
 *   $creator The name of the creator of the object
 * @return boolean
 *   FALSE if the name of the creator could not be retrieved.
 */
function islandora_workflow_get_object_creator($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $params = array(
    'query' => array(array('property' =>'pid', 'operator' => 'eq', 'value' => $object_id)),
    'resultFields' => array('pid', 'ownerId'),
    'maxResults' => 1,
  );
  $response = Fedora_Item::soap_call('findObjects', $params);

  if (!$response) {
    return FALSE;
  }
  return $response->result->resultList->objectFields->ownerId;
}

/**
 * Get the current Fedora state of an object.
 *
 * uses object properties
 *
 * @param string $object_id
 *   the Fedora PID of the object to check the status of
 *
 * @return string
 *   $state The string reperesenting the state of the object in fedora
 * @return boolean
 *   FALSE if the state of the object could not be retrieved.
 */
function islandora_workflow_get_object_fedora_state($object_id) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $params = array(
    'query' => array(array('property' =>'pid', 'operator' => 'eq', 'value' => $object_id)),
    'resultFields' => array('pid', 'state'),
    'maxResults' => 1,
  );
  $response = Fedora_Item::soap_call('findObjects', $params);

  if (!$response) {
    return FALSE;
  }
  return $response->result->resultList->objectFields->state;
}

/**
 * Get the current workflow state for an object.
 *
 * uses RELS-EXT
 *
 * @param string $object_id
 *   the Fedora PID of the object to check the status of
 *
 * @return string
 *   $state The string reperesenting the state of the object in the workflow
 * @return boolean
 *   FALSE if no workflow state has been set.
 */
function islandora_workflow_get_object_workflow_state($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $islandora_workflow_namespace = 'info:islandora/islandora-system:def/islandora_workflow#';

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  $workflow_relationships = $item->get_rdf_relationships($islandora_workflow_namespace);
  if (!empty($workflow_relationships)) {
    return $workflow_relationships['state'][0];
  }
  return FALSE;

}

/**
 * Get the current assignee for a Fedora object tracked by workflow.
 *
 * uses RELS-EXT
 *
 * @param string $object_id
 *   the Fedora PID of the object to check the status of
 *
 * @return string
 *   $assignee The string reperesenting the drupal
 *   user's name, or false if none was set
 */
function islandora_workflow_get_object_assignee($object_id) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  $namespace = 'info:islandora/islandora-system:def/islandora_workflow#';
  $workflow_relationships = $item->get_rdf_relationships($namespace);

  if (!empty($workflow_relationships['assignee'])) {
    foreach ($workflow_relationships['assignee'] as $assignee) {
      if (!empty($assignee)) {
        return $assignee;
      }
    }
  }
  return FALSE;
}

/**
 * This function will set the indicated relationship on the indicated object.
 *
 * It will create or replace the relationship  as apropriate.
 * The function currently only does literals.
 *
 * @param string $object_id
 *   the Fedora PID of the object whos RELS-EXT or RELS-INT will be modified
 * @param string $relationship
 *   the relationship to set
 * @param string $target
 *   a literal or Fedora PID string (object of the relationship)
 * @param string $subject
 *   This defaults to the object's pid in the REST
 *   interface, only specify if a datastream
 *
 * @return object
 *   the response from fedora for adding/modifying the relationship
 */
function islandora_workflow_set_object_relationship($object_id, $relationship, $target, $subject = NULL, $value_type = RELS_TYPE_PLAIN_LITERAL) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $namespace = 'info:islandora/islandora-system:def/islandora_workflow#';

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }

  if ($subject) {
    // Subject is an internal one - use RELS-INT.
    $item->purge_dsid_relationships($subject, $relationship, NULL, $namespace, $value_type);
    return $item->add_dsid_relationship($subject, $relationship, $target, $namespace, $value_type);
  }
  else {
    // External relationship - use RELS-EXT.
    $item->purge_relationships($relationship, NULL, $namespace, $value_type);
    return $item->add_relationship($relationship, $target, $namespace, $value_type);
  }
}

/**
 * Get the parent collection of an object.
 *
 * This function looks for the <isMemberOf> and <isMemberOfCollection>
 * relationships, in that order, in trying to determine a parent collection.
 *
 * If the object has more than one parent, only one will be returned.
 * @TODO determine the order (if any) which determines which parent is returned.
 *
 * @param string $object_id
 *   The PID of the object to get the parent of
 *
 * @return mixed
 *   $parent The PID of the parent object
 * @return boolean
 *   FALSE if no parent found
 */
function islandora_workflow_get_object_parent($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  $parents = $item->get_relationships('isMemberOf');
  if (!empty($parents)) {
    return $parents[0]['object'];
  }
  return FALSE;
}

/**
 * Set a workflow note on an object.
 *
 * From and timestamp are generated from the current user/time
 *
 * @param string $object_id
 *   The PID of the object to set the note on
 * @param string $subject
 *   The subject of the message
 * @param string $body
 *   The content of the message
 *
 * @return mixed
 *   the response from fedora_api for adding/modifying the
 *   datastream, or FALSE if the datastream was missing
 */
function islandora_workflow_set_object_note($object_id, $subject, $body) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  global $user;

  $item = new Fedora_Item($object_id);
  $workflow_parser = new DOMDocument();

  if (!isset($item->datastreams['islandora_workflow'])) {
    return FALSE;
  }
  // Get the DS xml in dom object.
  $workflow_string = $item->get_datastream_dissemination('islandora_workflow');
  $workflow_parser->loadXML($workflow_string);

  // Modify DS xml.
  /* Get elements to populate dynamicaly,
   * these lists are dynamic so no need to update later.*/
  $from_node_list = $workflow_parser->getElementsByTagName('from');
  $timestamp_node_list = $workflow_parser->getElementsByTagName('timestamp');
  $subject_node_list = $workflow_parser->getElementsByTagName('subject');
  $body_node_list = $workflow_parser->getElementsByTagName('body');
  $note_node_list = $workflow_parser->getElementsByTagName('note');
  $notes_node_list = $workflow_parser->getElementsByTagName('notes');

  /* Make sure that the notes and note elements
   * are present and add them if they are not.*/
  if (!$notes_node_list->item(0)) {
    $notes = $workflow_parser->createElement('notes');
    $workflow_node_list = $workflow_parser->getElementsByTagName('workflow');
    $workflow_node_list->item(0)->appendChild($notes);

  }
  if (!$note_node_list->item(0)) {
    $note = $workflow_parser->createElement('note');
    $notes_node_list->item(0)->appendChild($note);
  }

  // Create from node.
  $from = $workflow_parser->createElement('from');
  $from->appendChild($workflow_parser->createTextNode($user->name));
  if ($from_node_list->item(0)) {
    foreach ($from_node_list as $from_element) {
      $from_element->parentNode->replaceChild($from, $from_element);
    }
  }
  else {
    $note_node_list->item(0)->appendChild($from);
  }

  // Create timestamp.
  $timestamp = $workflow_parser->createElement('timestamp');
  $timestamp->appendChild($workflow_parser->createTextNode(date('c')));
  if ($timestamp_node_list->item(0)) {
    foreach ($timestamp_node_list as $timestamp_element) {
      // Insert text node.
      $timestamp_element->parentNode->replaceChild($timestamp, $timestamp_element);
    }
  }
  else {
    $note_node_list->item(0)->appendChild($timestamp);
  }

  // Create subject node.
  $subject_new_element = $workflow_parser->createElement('subject');
  $subject_new_element->appendChild($workflow_parser->createTextNode($subject));
  if ($subject_node_list->item(0)) {
    foreach ($subject_node_list as $subject_element) {
      // Insert text node.
      $subject_element->parentNode->replaceChild($subject_new_element, $subject_element);
    }
  }
  else {
    $note_node_list->item(0)->appendChild($subject_new_element);
  }


  // Create body node.
  $body_new_element = $workflow_parser->createElement('body');
  $body_new_element->appendChild($workflow_parser->createTextNode($body));
  if ($body_node_list->item(0)) {
    foreach ($body_node_list as $body_element) {
      // Insert text node.
      $body_element->parentNode->replaceChild($body_new_element, $body_element);
    }
  }
  else {
    $note_node_list->item(0)->appendChild($body_new_element);
  }

  islandora_workflow_set_object_relationship($object_id, 'has_note_subject', $subject, 'islandora_workflow');

  // Set the new DS content.
  return $item->modify_datastream(
      $workflow_parser->saveXML(),
      $ds_id = 'islandora_workflow',
      $ds_label = 'workflow',
      $mimetype = 'text/xml'
  );
}

/**
 * Retrieve the subject of the workflow note on an object (if present).
 *
 * @param string $object_id
 *   The PID of the object to get the note for
 *
 * @return mixed
 *   The contents of the object's note's subject,
 *   or FALSE if the datastream was missing
 */
function islandora_workflow_get_object_note_subject($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  $iw = $item->get_datastream_dissemination('islandora_workflow');
  if (!$iw) {
    return FALSE;
  }
  $dom = new DOMDocument();
  $dom->loadXML($iw);
  return $dom->getElementsByTagName('subject')->item(0)->nodeValue;
}

/**
 * Get the body of the workflow note on an object.
 *
 * @param string $object_id
 *   The PID of the object to get the note for
 *
 * @return mixed
 *   The contents of the object's note's body,
 *   or FALSE if the datastream was missing
 */
function islandora_workflow_get_object_note_body($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  $iw = $item->get_datastream_dissemination('islandora_workflow');
  if (!$iw) {
    return FALSE;
  }
  $dom = new DOMDocument();
  $dom->loadXML($iw);
  return $dom->getElementsByTagName('body')->item(0)->nodeValue;

}

/**
 * Get the workflow associated times for an object.
 *
 * @param string $object_id
 *   The pid of the object to return the timestamps for
 *
 * @return array
 *   $times An array of the objects timestamps, the 'created'
 *   element is creation while the 'modified' is the last workflow progression.
 */
function islandora_workflow_get_object_state_time_stamp($object_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  $iw = $item->get_datastream_dissemination('islandora_workflow');
  if (!$iw) {
    return FALSE;
  }
  $times = array();
  $dom = new DOMDocument();
  $dom->loadXML($iw);
  $modified_times = $dom->getElementsByTagName('when_last_modified');
  foreach ($modified_times as $modified_time) {
    $times['islandora_workflow_modified'] = format_date(strtotime($modified_time->nodeValue), 'small');
  }
  return $times;
}

/**
 * Get all users with a specific role.
 *
 * @param int $rid
 *   the Drupal role ID
 *
 * @return array
 *   $user_ids An array of user IDs.
 */
function islandora_workflow_get_users_with_role($rid) {
  $user_ids = array();
  $result = db_query('SELECT uid FROM {users_roles} WHERE rid = %d', $rid);
  while ($row = mysql_fetch_assoc($result)) {
    $user_ids[] = $row['uid'];
  }
  return $user_ids;
}
