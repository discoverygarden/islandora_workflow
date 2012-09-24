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

  GLOBAL $user;

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
  * Get a list of all of the Islandora collections in a Fedora repository.
  * @author
  *   William Panting
  *
  * @return array
  *   $collection_list an associated array of collection pids and names
  */
function islandora_workflow_get_all_collections() {

  $collection_list = array();

  // Read in the ITQL query for getting collections.
  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/collection_query.txt';
  $query_file_handle = fopen($query_file_name, "r");
  $query_string = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  // Make query.
  $collection_list = islandora_workflow_get_related_objects($query_string);
  // Strip out non-applicable collections via namespace.
  $collection_list = islandora_workflow_limit_collections_by_namespace($collection_list);
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

  // Get info for query.
  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/member_query.txt';
  $query_file_handle = fopen($query_file_name, "r");
  $query = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  $limit = -1;
  $offset = 0;
  $type = 'sparql';
  $query_url = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');

  // Inject collection value into the query.
  $pattern = '/(REGEX.+collection_object.+?\')(\.)(\')/';
  $replacement = '\1' . $collection_id .'\3';

  $query = preg_replace($pattern, $replacement, $query);

  // Run query.
  /*
  $query_url .= "?type=tuples&flush=TRUE&format=Sparql". (($limit > 0)? "&limit=$limit" : "") ."&offset=$offset&lang=$type&query=". urlencode($query);
  $query_return_string = do_curl($query_url);
  if ($query_return_string) {
    $query_return_dom = DOMDocument::loadXML($query_return_string);
  }
   */

  $query_return_json = query_resource_index($query, $limit, $offset, $format = 'json');

  if ($query_return_json) {
    $query_return = json_decode($query_return_json);
    $results = $query_return->results;
    $return_results = array();
    foreach ($results as $result) {
      $index = preg_replace('/^info:fedora\//', '', $result->member_object);
      $temp_result = (array) $result;
      $temp_result['islandora_workflow_modified'] = $temp_result['timestamp'];
      unset($temp_result['timestamp']);
      $return_results[$index] = $temp_result;
    }
    return $return_results;
  }
  return array();

  /*
  // Format query results for use (xml->array).
  $result_nodes = $query_return_dom->getElementsByTagName('result');
  $result_nodes_length = $result_nodes->length;
  if ($result_nodes_length > 0) {
    for ($i = 0; $i < $result_nodes_length; $i++) {
      $result_node = $result_nodes->item($i);
      $label_element = $result_node->getElementsByTagName('label')->item(0);
      $assignee_element = $result_node->getElementsByTagName('assignee')->item(0);
      $state_element = $result_node->getElementsByTagName('state')->item(0);
      $member_object_element = $result_node->getElementsByTagName('member_object')->item(0);
      $created_date_element = $result_node->getElementsByTagName('created_date')->item(0);
      $owner_element = $result_node->getElementsByTagName('owner')->item(0);
      $subject_element = $result_node->getElementsByTagName('subject')->item(0);
      $timestamp_element = $result_node->getElementsByTagName('timestamp')->item(0);

      $member_object = $member_object_element->getAttribute('uri');
      // Need to extract the PID: info:fedora/ is 12 chars.
      $member_object = substr($member_object, 12, strlen($member_object));

      $member_data[$member_object] = array();

      if ($label_element) {
        $label = $label_element->nodeValue;
        // The label may not be pupulated.
        if ($label) {
          $member_data[$member_object]['label'] = $label;
        }
      }
      if ($assignee_element) {
        $assignee = $assignee_element->nodeValue;
        // The assignee may not be populated.
        if ($assignee) {
          $member_data[$member_object]['assignee'] = $assignee;
        }
      }
      if ($state_element) {
        $state = $state_element->nodeValue;
        $member_data[$member_object]['state'] = $state;
      }
      if ($created_date_element) {
        $created_date = $created_date_element->nodeValue;
        $member_data[$member_object]['created_date'] = $created_date;
      }
      if ($owner_element) {
        $owner = $owner_element->nodeValue;
        $member_data[$member_object]['owner'] = $owner;
      }
      if ($timestamp_element) {
        $timestamp = $timestamp_element->nodeValue;
        $member_data[$member_object]['islandora_workflow_modified'] = $timestamp;
      }
      if ($subject_element) {
        $subject = $subject_element->nodeValue;
        $member_data[$member_object]['subject'] = $subject;
      }
    }
    return $member_data;
  }
  return FALSE;
   * 
   */
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
  * Perform an ITQL query on the Fedora resource index.
  *
  * @param string $itql_query
  *   An ITQL query to use for searching the index
  *
  * @return array
  *   $list_of_objects a nice array of objects
  */
function islandora_workflow_get_related_objects($itql_query) {
  module_load_include('inc', 'fedora_repository', 'CollectionClass');
  $collection_class = new CollectionClass();
  $query_results = $collection_class->getRelatedItems(NULL, $itql_query);
  $list_of_objects = islandora_workflow_itql_to_array($query_results);
  return $list_of_objects;
}

/**
 * Transform a ITQL query result into an array.
 *
 * Returns an associative array with PID => label key/value pairs.
 *
 * @author
 *   Paul Pound
 * @author
 *   William Panting
 *
 * @param string $query_results
 *   The ugly string version
 *
 * @return array
 *   $list_of_objects The well formed array version
 */
function islandora_workflow_itql_to_array($query_results) {
  try {
    $xml = new SimpleXMLElement($query_results);
  } catch (Exception $e) {
    drupal_set_message(t('Error getting list of collection objects !e', array('!e' => $e->getMessage())), 'error');
    return;
  }
  $list_of_objects = array();
  foreach ($xml->results->result as $result) {
    $a = $result->object->attributes();
    $temp = $a['uri'];
    $object = substr($temp, strrpos($temp, '/') + 1);
    $title = $result->title;
    $list_of_objects[$object] = (string) $title;
  }
  return $list_of_objects;
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
  module_load_include('object.inc', 'islandora_fedora_api');
  $object = new FedoraObject($object_id);

  // Attach default workflow.
  $default_xml_file_name = drupal_get_path('module', 'islandora_workflow') . '/start_workflow.xml';
  $workflow_parser = new DOMDocument();
  $workflow_parser->load($default_xml_file_name);
  $object->addDatastream('islandora_workflow',
    NULL,
    $workflow_parser->saveXML(),
    array(
      'dsLabel' => 'workflow',
      'mimeType' => 'text/xml',
      'controlGroup' => 'X',
      'logMessage' => 'Adding islandora_workflow datastream(editorial workflow tracking).',
    )
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

  // Permission check.
  $tmp = islandora_workflow_user_object_permission_check($object_id);
  if ($tmp != 'islandora_workflow_Editor' && $tmp != 'islandora_workflow_Manager') {
    drupal_set_message(t('The object %pid could not be rejected, you do not have the necessary permissions.', array('%pid' => $object_id)));
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
        drupal_set_message(t('The object %pid could not be rejected, you do not have XACML access to the object.', array('%pid' => $object_id)));
        return FALSE;
      }
    }

    // Send out email to creator.
    $user_name = islandora_workflow_get_object_creator($object_id);
    if ($user_name) {
      // Some objects may not have been created by a drupal user.
      $object = new Fedora_Item($object_id);
      // Optional parameters for mail hook.
      $recipient = user_load(array('name' => $user_name));
      $address = $recipient->mail;
      rules_invoke_event('islandora_workflow_rules_reject_object', $object);
    }
    else {
      drupal_set_message(t('No email sent for the rejection of: %pid', array('%pid' => $object_id)));
    }
    return (islandora_workflow_set_object_workflow_state($object_id, 'rejected'));
  }
  else {
    drupal_set_message(t('The object %pid could not be rejected, it was not in an appropriate workflow state.', array('%pid' => $object_pid)));
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

  // Permission check.
  $tmp = islandora_workflow_user_object_permission_check($object_id);
  if ($tmp != 'islandora_workflow_Editor' && $tmp != 'islandora_workflow_Manager') {
    drupal_set_message(t('The object %pid could not be approved, you do not have the necessary permissions.', array('%pid' => $object_pid)));
    return FALSE;
  }

  $workflow_state = islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state != 'published' && $workflow_state != 'approved') {

    // Send email to all managers.
    $collection = islandora_workflow_get_object_parent($object_id);
    $manager_list = islandora_workflow_get_all_users_with_collection_permissions(
      array($collection), $permission_level = 'islandora_workflow_Manager');
    $emails = get_users_email_by_name($manager_list[$collection]);
    $emails = format_email_array_to_string($emails);
    $object = new Fedora_Item($object_id);
    // drupal_mail('islandora_workflow', 'approve', $emails, language_default(), $params);
    rules_invoke_event('islandora_workflow_rules_approve_object', $object);
    return (islandora_workflow_set_object_workflow_state($object_id, 'approved'));
  }
  else {
    drupal_set_message(t('The object %pid could not be approved, it was not in an appropriate workflow state.', array('%pid' => $object_id)));
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
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');

  // Permission check.
  if (islandora_workflow_user_object_permission_check($object_id) != 'islandora_workflow_Manager') {
    drupal_set_message(t('The object %pid could not be published, you do not have the necessary permissions.', array('%pid' => $object_id)));
    return FALSE;
  }

  if (islandora_workflow_get_object_workflow_state($object_id) != 'published') {
    islandora_workflow_set_object_workflow_state($object_id, 'published');
    // Make the object viewable.
    module_load_include('object.inc', 'islandora_fedora_api');
    $object = new FedoraObject($object_id);
    return ($object->__set('state', 'A'));
  }
  else {
    drupal_set_message(t('The object %pid could not be published, it was not in an appropriate workflow state.', array('%pid' => $object_id)));
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

  // Permission check.
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object %pid could not be submitted, you do not have the necessary permissions.', array('%pid' => $object_id)));
    return FALSE;
  }

  $workflow_state = islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state == 'created' || $workflow_state == 'rejected') {

    // Send email to all editors.
    $collection = islandora_workflow_get_object_parent($object_id);
    $editor_list = islandora_workflow_get_all_users_with_collection_permissions(
      array($collection), $permission_level = 'islandora_workflow_Editor');
    $emails = get_users_email_by_name($editor_list[$collection]);
    $emails = format_email_array_to_string($emails);
    $object = new Fedora_Item($object_id);
    rules_invoke_event('islandora_workflow_rules_submit_object', $object);
    return (islandora_workflow_set_object_workflow_state($object_id, 'submitted'));
  }
  else {
    drupal_set_message(t('The object %pid could not be submitted, it was not in an appropriate workflow state.', array('%pid' => $object_id)));
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

  // Permission check.
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object %pid could not be assigned, you do not have the necessary permissions.', array('%pid' => $object_id)));
    return FALSE;
  }
  // Send out email to assignee.
  $object = new Fedora_Item($object_id);
  // Optional parameters for mail hook.
  $recipient = user_load(array('name' => $user_name));
  $address = $recipient->mail;
  rules_invoke_event('islandora_workflow_rules_assign_object', $object);
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
  module_load_include('object.inc', 'islandora_fedora_api');
  // For getting an object.
  module_load_include('inc', 'islandora_fedora_api', 'islandora_fedora_api.exception');

  $object = new FedoraObject($object_id);
  $workflow_parser = new DOMDocument();
  global $user;
  $object_datastreams = $object->get_datastreams();

  // Get the collection of this object.
  $collection = islandora_workflow_get_object_parent($object_id);

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

    if (isset($object_datastreams['POLICY'])) {
      $object_datastreams['POLICY']->content = $xacml->getXmlString();
    }
    else {
      $object->addDatastream('POLICY', NULL, $xacml->getXmlString(), array('mimeType' => 'text/xml', 'controlGroup' => 'X'));
    }
  }
  else {
    try {
      /* XACML is disabled. delete the xacml datastream if
       * it exists before it starts causing problems.*/
      if (isset($object_datastreams['POLICY'])) {
        $object_datastreams['POLICY']->purge();
      }
      $collection_object = new FedoraObject($collection);
      $collection_datastreams = $collection_object->get_datastreams();

      if (isset($collection_datastreams['CHILD_SECURITY'])) {
        $object->addDatastream('POLICY', NULL, $collection_datastreams['CHILD_SECURITY']->content, array(
          'dsLabel' => 'security policy',
          'mimeType' => 'text/xml',
          'controlGroup' => 'X',
          'logMessage' => 'Adding policy datastream.',
        ));
      }
    }
    catch (FedoraAPIRestException $e) {
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
  if (isset($object_datastreams['islandora_workflow'])) {
    // Build xml from DS.
    $workflow_datastream = $object_datastreams['islandora_workflow'];
    $workflow_string = $workflow_datastream->__get('content');
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
    $workflow_datastream->__set('content', $workflow_parser->saveXML());
  }
  else {
    // Build xml from default if there is no DS present.
    $default_xml_file_name = drupal_get_path('module', 'islandora_workflow') . '/start_workflow.xml';
    $workflow_parser = new DOMDocument();
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
    $object->addDatastream('islandora_workflow', NULL, $workflow_parser->saveXML(), array(
      'dsLabel' => 'workflow',
      'mimeType' => 'text/xml',
      'controlGroup' => 'X',
      'logMessage' => 'Adding islandora_workflow datastream (workflow tracking).',
    ));
  }

  // Change the rels-int.
  islandora_workflow_set_object_relationship($object_id, 'has_workflow_timestamp', date('c'), 'info:fedora/' . $object_id . '/islandora_workflow');

  /* Make sure that the object's fedora
   * state is going to be set to I if the object is still in workflow.*/
  if ($state != 'published' && islandora_workflow_get_object_fedora_state($object_id) != 'I') {
    $object->__set('state', 'I');
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
  try {
    module_load_include('object.inc', 'islandora_fedora_api');
    $object = new FedoraObject($object_id);
    $label = $object->__get('label');
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }
  return $label;
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

  // Permission check.
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object %pid could not be deleted, you do not have the necessary permissions.', array('%pid' => $object_id)));
    return FALSE;
  }

  module_load_include('object.inc', 'islandora_fedora_api');
  $object = new FedoraObject($object_id);
  return ($object->__set('state', 'D'));
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

  module_load_include('object.inc', 'islandora_fedora_api');
  try {
    $object = new FedoraObject($object_id);
    $creator = $object->__get('ownerId');
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }
  return $creator;
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
  module_load_include('object.inc', 'islandora_fedora_api');

  try {
    $object = new FedoraObject($object_id);
    $state = $object->__get('state');
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }
  return $state;
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

  module_load_include('raw.inc', 'islandora_fedora_api');
  // For getting an object.
  $apim_object = new FedoraAPIM();

  try {
    $relationships = $apim_object->getRelationships($object_id, 'info:islandora/islandora-system:def/islandora_workflow#state');
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }

  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);

  $state_elements = $relationships_parser->getElementsByTagNameNS('info:islandora/islandora-system:def/islandora_workflow#', 'state');

  $state = $state_elements->item(0)->firstChild;
  if ($state) {
    $state = $state->nodeValue;
    return $state;
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
  module_load_include('raw.inc', 'islandora_fedora_api');
  // For getting an object.
  $apim_object = new FedoraAPIM();

  try {
    $relationships = $apim_object->getRelationships($object_id, 'info:islandora/islandora-system:def/islandora_workflow#assignee');
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }

  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);

  $assignee_elements = $relationships_parser->getElementsByTagNameNS('info:islandora/islandora-system:def/islandora_workflow#', 'assignee');

  $assignee = $assignee_elements->item(0)->firstChild;
  if ($assignee) {
    $assignee = $assignee->nodeValue;
    return $assignee;
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
 *   the Fedora PID of the object whos rels-ext will be modified
 * @param string $relationship_in
 *   the relationship to set
 * @param string $target
 *   a litteral or Fedora PID string (object of the relationship)
 * @param string $subject
 *   This defaults to the object's pid in the REST
 *   interface, only specify if a datastream
 *
 * @return object
 *   the response from fedora for adding/modifying the relationship
 */
function islandora_workflow_set_object_relationship($object_id, $relationship_in, $target, $subject = NULL) {
  $islandora_workflow_namespace = 'info:islandora/islandora-system:def/islandora_workflow#';
  module_load_include('raw.inc', 'islandora_fedora_api');
  $apim_object = new FedoraAPIM();

  // Get existing relationshp.
  $relationships = $apim_object->getRelationships($object_id, $islandora_workflow_namespace . $relationship_in, $subject);

  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);
  $relationship_elements = $relationships_parser->getElementsByTagNameNS($islandora_workflow_namespace, $relationship_in);
  $current_relationship = NULL;
  $relationship = $relationship_elements->item(0);

  if (!empty($relationship)) {
    foreach ($relationship->childNodes as $relationship_text_node) {
      $current_relationship = $relationship_text_node->nodeValue;
      // Clear current relationship.
      $purge_response = $apim_object->purgeRelationship($object_id, $islandora_workflow_namespace . $relationship_in, $current_relationship, array('isLiteral' => 'true', 'subject' => $subject));
    }
  }
  // Set new relationship.
  $apim_object->addRelationship($object_id, $islandora_workflow_namespace . $relationship_in, $target, array('isLiteral' => 'true', 'subject' => $subject));
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
  $parent_relationship = 'isMemberOf';
  $parent_relationship_namespace = 'info:fedora/fedora-system:def/relations-external#';
  module_load_include('raw.inc', 'islandora_fedora_api');
  $apim_object = new FedoraAPIM();
  $relationships_parser = new DOMDocument();
  $parent = FALSE;

  // Get relation ship data.
  try {
    $relationships = $apim_object->getRelationships($object_id, $parent_relationship_namespace . $parent_relationship);
    // Grab realtionship.
    $relationships_parser->loadXML($relationships->data);
    $relationship_elements = $relationships_parser->getElementsByTagNameNS($parent_relationship_namespace, $parent_relationship);
    $relationship = $relationship_elements->item(0);
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }

  // Handle second collecion memberships string if the first wasn't found.
  if (empty($relationship)) {
    $parent_relationship = 'isMemberOfCollection';
    try {
      $relationships = $apim_object->getRelationships($object_id, $parent_relationship_namespace . $parent_relationship);
      // Grab relationship.
      $relationships_parser->loadXML($relationships->data);
      $relationship_elements = $relationships_parser->getElementsByTagNameNS($parent_relationship_namespace, $parent_relationship);
      $relationship = $relationship_elements->item(0);
    }
    catch (FedoraAPIRestException $e) {
      return FALSE;
    }
  }

  // Handle relationship data.
  if (!empty($relationship)) {
    $parent = $relationship->getAttributeNS('http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'resource');
    // Cut out 'info:fedora/'.
    if (substr($parent, 0, 12) == 'info:fedora/') {
      $parent = substr($parent, 12, strlen($parent));
    }
  }
  return $parent;
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

  module_load_include('object.inc', 'islandora_fedora_api');
  // For getting an object.
  $object = new FedoraObject($object_id);
  $workflow_parser = new DOMDocument();
  global $user;
  $object_datastreams = $object->get_datastreams();
  if (!isset($object_datastreams['islandora_workflow'])) {
    return FALSE;
  }
  // Get the DS xml in dom object.
  $workflow_datastream = $object_datastreams['islandora_workflow'];
  $workflow_string = $workflow_datastream->__get('content');
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

  islandora_workflow_set_object_relationship($object_id, 'has_note_subject', $subject, 'info:fedora/' . $object_id . '/islandora_workflow');

  // Set the new DS content.
  return ($workflow_datastream->__set('content', $workflow_parser->saveXML()));
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

  module_load_include('object.inc', 'islandora_fedora_api');
  try {
    $object = new FedoraObject($object_id);
    $workflow_parser = new DOMDocument();
    global $user;
    $object_datastreams = $object->get_datastreams();
    if (!isset($object_datastreams['islandora_workflow'])) {
      return FALSE;
    }
    // Get the DS xml in dom object.
    $workflow_datastream = $object_datastreams['islandora_workflow'];
    $workflow_string = $workflow_datastream->__get('content');
    $workflow_parser->loadXML($workflow_string);
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }
  $subject = '';

  $subject_node_list = $workflow_parser->getElementsByTagName('subject');
  foreach ($subject_node_list as $subject_element) {
    // Get the text.
    $subject = $subject_element->nodeValue;
  }
  return $subject;
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

  module_load_include('object.inc', 'islandora_fedora_api');

  try {
    $object = new FedoraObject($object_id);
    $workflow_parser = new DOMDocument();
    global $user;
    $object_datastreams = $object->get_datastreams();
    if (!isset($object_datastreams['islandora_workflow'])) {
      return FALSE;
    }
    // Get the DS xml in dom object.
    $workflow_datastream = $object_datastreams['islandora_workflow'];
    $workflow_string = $workflow_datastream->__get('content');
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }

  $workflow_parser->loadXML($workflow_string);

  $body = '';

  $body_node_list = $workflow_parser->getElementsByTagName('body');

  foreach ($body_node_list as $body_element) {
    // Get the text.
    $body = $body_element->nodeValue;
  }

  return $body;
}

/**
 * Get the registered email addresses for a set of named users.
 *
 * @author
 *   William Panting
 *
 * @param array $users_names
 *   an array containing all the users names to find emails for
 *
 * @return array
 *   $users_emails An associative array of user_name=>user_email
 */
function get_users_email_by_name($users_names) {
  $users_emails = array();
  if (is_array($users_names)) {
    foreach ($users_names as $user_name) {
      $user_object = user_load(array('name' => $user_name));
      $email = $user_object->mail;
      $users_emails[$user_name] = $email;
    }
  }
  return $users_emails;
}

/**
 * Transform an associative array of email addresses to a delimited string.
 *
 * Given an array with username/email key/value pairs,
 * return a semicolon-delimited string suitable for using in drupal_mail().
 *
 * @see drupal_mail
 *
 * @author
 *   William Panting
 *
 * @param array $emails_array
 *   The array of email strings to concatenate
 *
 * @return string
 *   $emails_string The formatted string containing all of the emails
 */
function format_email_array_to_string($emails_array) {
  $emails_string = '';
  if (is_array($emails_array)) {
    // For telling if this is the first iteration of the loop.
    $is_first = TRUE;

    foreach ($emails_array as $key => $email) {
      if ($is_first == TRUE) {
        // First email in list.
        $emails_string = $email;
        $is_first = FALSE;
      }
      else {
        // Emails get a comma delimiter.
        $emails_string = $emails_string . ', ' . $email;
      }
    }
  }
  return $emails_string;
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

  $times = array();

  module_load_include('object.inc', 'islandora_fedora_api');
  $object = new FedoraObject($object_id);

  $object_datastreams = $object->get_datastreams();

  if (isset($object_datastreams['islandora_workflow'])) {
    // Build xml from DS.
    $workflow_parser = new DOMDocument();
    $workflow_datastream = $object_datastreams['islandora_workflow'];
    $workflow_string = $workflow_datastream->__get('content');
    $workflow_parser->loadXML($workflow_string);
    $when_last_modified_array = $workflow_parser->getElementsByTagName('when_last_modified');

    foreach ($when_last_modified_array as $when_last_modified_element) {
      // Get timestamp.
      $times['islandora_workflow_modified'] = strtotime($when_last_modified_element->nodeValue);
      $times['islandora_workflow_modified'] = format_date($times['islandora_workflow_modified'], $type = 'small');
    }
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
