<?php
/**
 * @file
 * @author: William Panting
 * @date: 02/06/2011
 * This is a collection of helper functions and such for the islandora_workflow module.
 * All the workflow wrapper functions are there so that in the future we can add specific error/permission reports and logic
 * @todo
 *   permission checks on workflow operations need to be implemented
 */

/**
 * This function uses the get_all_collections() function in the case of an admin user
 * but otherwise will use the supplied permission string to search
 * the drupal database table 'islandora_workflow_permissions' to compile a list of 
 * collections that the user has the supplied permission to 
 * This is also handy to determin if the user has any permissions at that lvl:
 * if (islandora_workflow_get_current_users_collections($perm)!=FALSE) {}
 * if (islandora_workflow_get_current_users_collections()) {}
 * @param string $permission_level
 *   the permission level we are looking for if it is the string 'all' it will get all collections the user has any access to
 * @return $collection_list
 *   an associated array of collection pids and names, 
 * @return boolean
 *   a false boolean if they don't have the indicated perms
 */
function islandora_workflow_get_current_users_collections($permission_level='islandora_workflow_Manager') {
  //init
  $permitted_collections=array();
  $collection_list=array();
  if (user_access('islandora_workflow_Administrator') || user_access('administer')) {//admin show all
    $collection_list=islandora_workflow_get_all_collections();
  }
  elseif (user_access($permission_level) || $permission_level=='all') {//manager show permitted
    $collection_list=islandora_workflow_get_all_collections();
    //figure out owned collections
    //role perms
    $users_role_collection_permissions=islandora_workflow_get_current_users_role_permissions();
    foreach ($users_role_collection_permissions as $role_id => $collections) {
      foreach ($collections as $collection_id => $permission) {
        if ($permission_level=='all') {//all available permissions
          $permitted_collections[]=$collection_id;
        }
        elseif ($permission==$permission_level) {
          $permitted_collections[]=$collection_id;
        }
      }
    }
    //user perms
    $users_user_collection_permissions=islandora_workflow_get_current_users_user_permissions();
    foreach ($users_user_collection_permissions as $role_id => $collections) {
      foreach ($collections as $collection_id => $permission) {
        if ($permission_level=='all') {//all available permissions
          $permitted_collections[]=$collection_id;
        }
        elseif ($permission==$permission_level) {
          $permitted_collections[]=$collection_id;
        }
      }
    }
    //only send back owned collection pids and names
    foreach ($collection_list as $collection_pid => $collection_name) {
      if (!in_array($collection_pid, $permitted_collections)) {
        unset($collection_list[$collection_pid]);
      }
    }
  }
  else {
    return FALSE;
  }
  return $collection_list; 
}

/**
 * This function query's drupal's DB for all the collection permission entries by role
 * Used when displaying the current state for the permission page
 * @return array $perm_list
 *   list of permissions that roles have with the form $perm_list[$role_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_all_role_permissions() {
  $perm_list=array();
  
  //build query
  $query='SELECT role, collection, permission FROM {islandora_workflow_role_permissions}';
  $results=db_query($query);
  
  //build return array
  while ($row=db_fetch_array($results)) {
    $perm_list[$row['role']][$row['collection']]=$row['permission'];
  }

  return $perm_list;
}

/**
 * This function query's drupal's DB for all the collection permission entries for users
 * Used when displaying the current state for the permission page
 * @return array $perm_list
 *   list of all permisions that users have  with the form  $perm_list[$user_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_all_user_permissions() {
  $perm_list=array();
  
  //build query
  $query='SELECT user, collection, permission FROM {islandora_workflow_user_permissions}';
  $results=db_query($query);
  
  //build return array
  while ($row=db_fetch_array($results)) {
    $perm_list[$row['user']][$row['collection']]=$row['permission'];
  }

  return $perm_list;
}

/**
 * This is a helper function that will search the 
 * @param string $perm 
 *   the permission to search for
 * @return array $users 
 *   the list of users with the permission in the form users[user_id]=user_name
 */
function user_access_list($perm = NULL) {
  $uids = FALSE;
  if (isset($perm)) {
    $users_query = "SELECT u.uid, u.name FROM {users} u, {users_roles} ur, {permission} p
    WHERE        u.uid = ur.uid
    AND          ur.rid = p.rid
    AND          p.perm LIKE '%%%s%'
    ";
    $users_result = db_query($users_query, $perm);
    $users = array();
    while ($u = db_fetch_object($users_result)) {
      $users[$u->uid]=$u->name;
    }
  /*Drupal 6(note the 6) users_roles table does not record authenticated/anonymous/admin user
    *I need to know about authenticated users so, here we manualy check and add them to the list
    */
    $authenticated_users_query = "SELECT rid, perm FROM {permission}
      WHERE       rid = 2
      AND          perm LIKE '%%%s%'
      ";
    $authenticated_users_result = db_query($authenticated_users_query, $perm);

    while ($u = db_fetch_object($authenticated_users_result)) {
      $get_authenticated_users_query = "SELECT uid, name FROM {users}
        WHERE       uid > 0
        ";
      $get_authenticated_users_result = db_query($get_authenticated_users_query);
  
      while ($u = db_fetch_object($get_authenticated_users_result)) {
        $users[$u->uid]=$u->name;
      }
    }
  }
  return $users;
}

/**
 * This function query's drupal's DB for all the collection permission entries related to the current user
 * Most usefull for building the work portal, and also used for getting user's owned collections
 * @return array $perm_list
 *   list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_current_users_role_permissions() {
  //do islandora_workflow_get_all_role_permissions with restrictions on the roles to the list of roles the current user has
  $perm_list=array();
  global $user;//use $user->roles to get an array $roles[$role_id][$role_name]  $role_id is int
  
  //build query
  $query='SELECT role, collection, permission FROM {islandora_workflow_role_permissions}';
  $user_roles=$user->roles;
  
  $loop_count=0;
  foreach ($user_roles as $role_pid => $role_name) {
    if ($loop_count==0) {//if on the first role user the where clause
      $query=$query . ' WHERE role="' . $role_pid . '"';
    }
    else {
      $query=$query . ' OR role="' . $role_pid . '"';
    }
    $loop_count++;
  }
  $results=db_query($query);
  
  //build return array
  while ($row=db_fetch_array($results)) {
    $perm_list[$row['role']][$row['collection']] = $row['permission'];
  }
  return $perm_list;  
}

/**
* This function query's drupal's DB for all the collection permission entries related to the current user
 * Most usefull for building the work portal, and also used for getting user's owned collections
 * @return array $perm_list
 *   list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_current_users_user_permissions() {
  //do islandora_workflow_get_all_role_permissions with restrictions on the roles to the list of roles the current user has
  $perm_list=array();
  global $user;//use $user->roles to get an array $roles[$role_id][$role_name]  $role_id is int
  
  //build query
  $query='SELECT user, collection, permission FROM {islandora_workflow_user_permissions}';
  $user_id=$user->uid;
  $query=$query . ' WHERE user="' . $user_id . '"';
  $results=db_query($query);
  
  //build return array
  while ($row=db_fetch_array($results)) {
    $perm_list[$row['user']][$row['collection']] = $row['permission'];
  }
  return $perm_list;  
}

 /**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @return $collection_list
 *   an associated array of collection pids and names
 */
function islandora_workflow_get_all_collections() {
  
  $collection_list=array();
  
  //read in the itql query for getting collections
  $query_file_name=drupal_get_path('module', 'islandora_workflow') . '/collection_query.txt';
  $query_file_handle=fopen($query_file_name, "r");
  $query_string=fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  $collection_list=islandora_workflow_get_related_objects($query_string);
  return $collection_list;
}
 
 /**
 *This function executes a query on Fedora's resource index
 * @param string $itql_query
 *   A query to use for searching the index
 * @return array $list_of_objects
 *   a nice array of objects
 */
function islandora_workflow_get_related_objects($itql_query) {
  module_load_include('inc', 'fedora_repository', 'CollectionClass');
  $collection_class = new CollectionClass();
  $query_results = $collection_class->getRelatedItems(NULL, $itql_query);
  $list_of_objects = islandora_workflow_itql_to_array($query_results);
  return $list_of_objects;
}

/**
 *This function turns an itql result into a usefull array
 * @param string $query_results
 *   The ugly string version
 * @return array $list_of_objects
 *   The well formed array version
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
    $list_of_objects[$object] = (string) $title; //.' '.$object;
  }
  return $list_of_objects;
}

/**
 * This function will return a list of all roles that have islandora_workflow permissions
 * @return array $privledged_roles
 *   The list of roles
 */
function islandora_workflow_get_roles_with_module_permissions() {
   $privledged_roles=array();
   
   //use user_roles for each permission individualy
   $admin_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Administrator');
   $manager_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Manager');
   $editor_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Editor');
   $submitter_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Submitter');
   
   //modify the returned arrays to include the permission name, and merge into one array [role_pid][role_name][0...4]permission
   foreach ($admin_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Administrator';
   }
   
   foreach ($manager_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Manager';
   }
   
   foreach ($editor_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Editor';
   }
   
   foreach ($submitter_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Submitter';
   }
   
   return $privledged_roles;
}

/**
 * This function will return a list of all users that have islandora_workflow permissions
 * @return array $privledged_users
 *   The list of users
 */
function islandora_workflow_get_users_with_module_permissions() {
   $privledged_users=array();
   
   //use user_roles for each permission individualy
   $admin_users=user_access_list('islandora_workflow_Administrator');
   $manager_users=user_access_list('islandora_workflow_Manager');
   $editor_users=user_access_list('islandora_workflow_Editor');
   $submitter_users=user_access_list('islandora_workflow_Submitter');
   //modify the returned arrays to include the permission name, and merge into one array [role_pid][role_name][0...4]permission
   foreach ($admin_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Administrator';
   }
   
   foreach ($manager_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Manager';
   }
   
   foreach ($editor_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Editor';
   }
   
   foreach ($submitter_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Submitter';
   }
   return $privledged_users;
}

/**
 * This function will add a workflow datastream to a fedora object
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @return string $result
 *   the return mesage from fedora
 */
function islandora_workflow_init_workflow($object_id) {
  //init
  module_load_include('inc', 'fedora_repository', 'api/fedora_item'); //for getting an object
  $object= new Fedora_Item($object_id);
  $default_xml_file_name=drupal_get_path('module', 'islandora_workflow') . '/start_workflow.xml';
  $workflow_parser = new DOMDocument();
  $workflow_parser->load($default_xml_file_name);
  global $user;
  //get elements to populate dynamicaly
  $last_modified_by_array=$workflow_parser->getElementsByTagName('last_modified_by');
  $when_last_modified_array=$workflow_parser->getElementsByTagName('when_last_modified');
  
  //rels-ext
  module_load_include('object.inc', 'islandora_fedora_api'); //for getting an object
  $object= new FedoraObject($object_id);
  $object->__set('state', 'I');
  
  islandora_workflow_set_object_workflow_state($object_id, 'created');
  islandora_workflow_claim_object($object_id);
  
  //workflow DS
  foreach ($last_modified_by_array as $last_modified_by_element) {
    //create text node
    $last_modified_by=$workflow_parser->createTextNode($user->name);
    //insert text node
    $last_modified_by_element->appendChild($last_modified_by);
  }
  foreach ($when_last_modified_array as $when_last_modified_element) {
    //create text node
    $when_last_modified=$workflow_parser->createTextNode(date('c'));//date('c') is supposed to give back an xml friendly string
    //insert text node
    $when_last_modified_element->appendChild($when_last_modified);
  }
  
  $result=$object->addDatastream('islandora_workflow', NULL,  $workflow_parser->saveXML(), array('dsLabel' => 'workflow', 'mimeType' => 'text/xml', 'controlGroup' => 'X', 'logMessage' => 'Adding islandora_workflow datastream(workflow tracking).'));
  return $result;
}

/**
 * This function will move a fedora object to the rejected state
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @return string
 *   the return mesage from fedora
 * @return boolean
 *   false if the rejection was not attempted
 */
function islandora_workflow_reject_object($object_id) {
  
  //permission check
  $tmp=islandora_workflow_user_object_permission_check($object_id);
  if ($tmp!='islandora_workflow_Editor' && $tmp!='islandora_workflow_Manager') {
    drupal_set_message(t('The object '. $object_id .' could not be rejected, you do not have the necessary permissions.'));
    return FALSE;
  }
  
  islandora_workflow_get_object_parent($object_id);
  
  $workflow_state=islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state!='created' && $workflow_state!='rejected') {
    return (islandora_workflow_set_object_workflow_state($object_id, 'rejected'));
  }
  else {
    drupal_set_message(t('The object '. $object_id .' could not be rejected, it was not in an aporopriate workflow state.'));
    return FALSE;
  }
}

/**
 * This function will move a fedora object to the approved state
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @return string
 *   the return mesage from fedora
 * @return boolean
 *   false if the approval was not attempted
 */
function islandora_workflow_approve_object($object_id) {
  
  //permission check
  $tmp=islandora_workflow_user_object_permission_check($object_id);
  if ($tmp!='islandora_workflow_Editor' && $tmp!='islandora_workflow_Manager') {
    drupal_set_message(t('The object '. $object_id .' could not be approved, you do not have the necessary permissions.'));
    return FALSE;
  }
  
  $workflow_state=islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state!='published' && $workflow_state!='approved') {
    return (islandora_workflow_set_object_workflow_state($object_id, 'approved'));
  }
  else {
    drupal_set_message(t('The object '. $object_id .' could not be approved, it was not in an aporopriate workflow state.'));
    return FALSE;
  }
} 

/**
 * This function will move a fedora object to the published state
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @return string
 *   the return mesage from fedora [on changing state to 'A']
 * @return boolean
 *   false if the publication was not attempted
 */
function islandora_workflow_publish_object($object_id) {
  //permission check
  if (islandora_workflow_user_object_permission_check($object_id)!='islandora_workflow_Manager') {
    drupal_set_message(t('The object '. $object_id .' could not be published, you do not have the necessary permissions.'));
    return FALSE;
  }
  
  if (islandora_workflow_get_object_workflow_state($object_id)!='published') {
    islandora_workflow_set_object_workflow_state($object_id, 'published');
    //make the object viewable
    module_load_include('object.inc', 'islandora_fedora_api');
    $object= new FedoraObject($object_id);
    return ($object->__set('state', 'A'));
  }
  else {
    drupal_set_message(t('The object '. $object_id .' could not be published, it was not in an aporopriate workflow state.'));
    return FALSE;
  }
}

/**
 * This function will move a fedora object to the submitted state
 * [wrapper function]
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @return string
 *   the return mesage from fedora
 * @return boolean
 *   false if the submission was not attempted
 */
function islandora_workflow_submit_object($object_id) {
  //permission check
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object '. $object_id .' could not be submitted, you do not have the necessary permissions.'));
    return FALSE;
  }
  
  $workflow_state=islandora_workflow_get_object_workflow_state($object_id);
  if ($workflow_state=='created' || $workflow_state=='rejected') {
    return (islandora_workflow_set_object_workflow_state($object_id, 'submitted'));
  }
  else {
    drupal_set_message(t('The object '. $object_id .' could not be submitted, it was not in an aporopriate workflow state.'));
    return FALSE;
  }
}

/**
 * This function will assign the object to the current user state
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @return string
 *   the return mesage from fedora
 */
function islandora_workflow_claim_object($object_id) {
  global $user;
  return (islandora_workflow_assign_object($object_id, $user->name));
}

/**
 * This function will assign the fedora object to the indicated user
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @param string $user_name
 *   the name of the drupal user to assign the object to
 * @return string $result
 *   the return mesage from fedora
 */
function islandora_workflow_assign_object($object_id, $user_name) {
  //permission check
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object '. $object_id .' could not be assigned, you do not have the necessary permissions.'));
    return FALSE;
  }
  return (islandora_workflow_set_object_relationship($object_id, 'assignee', $user_name));
}

/**
 * This function will set the current state of workflow for an object.
 * uses RELS-EXT
 * @param string $object_id
 *   the fedora pid of the object to check the status of
 * @param string $state
 *   The string reperesenting the state of the object in the workflow
 * @return string $result
 *   the return mesage from fedora
 */
function islandora_workflow_set_object_workflow_state($object_id, $state) {
  //make sure that the object's fedora state is going to be set to I if the object is still in workflow
  if ($state!='published' && islandora_workflow_get_object_fedora_state($object_id)!='I') {
    module_load_include('object.inc', 'islandora_fedora_api'); //for getting an object
    $object= new FedoraObject($object_id);
    $object->__get('state', 'I');
  }
  return (islandora_workflow_set_object_relationship($object_id, 'state', $state));
}

/**
 * This function will do something that looks like deleteing it
 * @param string $object_id
 *   the fedora pid of the object to modify
 * @return string $result
 *   the return mesage from fedora
 */
function islandora_workflow_delete_object($object_id) {
  //permission check
  if (!islandora_workflow_user_object_permission_check($object_id)) {
    drupal_set_message(t('The object '. $object_id .' could not be deleted, you do not have the necessary permissions.'));
    return FALSE;
  }
  
  module_load_include('object.inc', 'islandora_fedora_api'); //for getting an object
  $object= new FedoraObject($object_id);
  return ($object->__set('state', 'D'));
}

/**
 * This function will get the user who created the object [may or may not be a durpal user]
 * uses object properties
 * @param string $object_id
 *   the fedora pid of the object to check the status of
 * @return string $creator
 *   The string reperesenting the creator of the object
 */
function islandora_workflow_get_object_creator($object_id) {
  //init
  module_load_include('object.inc', 'islandora_fedora_api'); //for getting an object
  $object= new FedoraObject($object_id);
  $creator=$object->__get('ownerId');
  return $creator;
}

/**
 * This function will get the current state of workflow for an object.
 * uses object properties
 * @param string $object_id
 *   the fedora pid of the object to check the status of
 * @return string $state
 *   The string reperesenting the state of the object in fedora
 */
function islandora_workflow_get_object_fedora_state($object_id) {
  //init
  module_load_include('object.inc', 'islandora_fedora_api'); //for getting an object
  $object= new FedoraObject($object_id);
  $state=$object->__get('state');
  return $state;
}

/**
 * This function will get the current state of workflow for an object.
 * uses RELS-EXT
 * @param string $object_id
 *   the fedora pid of the object to check the status of
 * @return $state
 *   The string reperesenting the state of the object in the workflow 
 * 	  or a boolean FALSE if it has not ben set
 */
function islandora_workflow_get_object_workflow_state($object_id) {
  //init
  module_load_include('raw.inc', 'islandora_fedora_api'); //for getting an object
  $apim_object= new FedoraAPIM();
  $relationships=$apim_object->getRelationships($object_id, 'info:islandora/islandora-system:def/islandora_workflow#state');
  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);

  $state_elements=$relationships_parser->getElementsByTagNameNS('info:islandora/islandora-system:def/islandora_workflow#', 'state');

  foreach ($state_elements->item(0)->childNodes as $state_node) {
    $state=$state_node->nodeValue;
    return $state;
  }
  
  return FALSE;
}

/**
 * This function will get the current assignee for a workflow object.
 * uses RELS-EXT
 * @param string $object_id
 *   the fedora pid of the object to check the status of
 * @return string $assignee
 *   The string reperesenting the drupal user's name, or false if none was set
 */
function islandora_workflow_get_object_assignee($object_id) {
  //init
  module_load_include('raw.inc', 'islandora_fedora_api'); //for getting an object
  $apim_object= new FedoraAPIM();
  $relationships=$apim_object->getRelationships($object_id, 'info:islandora/islandora-system:def/islandora_workflow#assignee');
  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);

  $assignee_elements=$relationships_parser->getElementsByTagNameNS('info:islandora/islandora-system:def/islandora_workflow#', 'assignee');

  foreach ($assignee_elements->item(0)->childNodes as $assignee_node) {
    $assignee=$assignee_node->nodeValue;
    return $assignee;
  }
  
  return FALSE;
}

/**
 * This function will set the indicated relationship on the indicated object.  
 * It will create or replace the relationship  as apropriate.
 * @param $object_id
 *   the fedora pid of the object whos rels-ext will be modified
 * @param string $relationship
 *   the relationship to set
 * @param string $target
 *   a litteral or fedora pid string
 * @return
 *   the response from fedora for adding/modifying the relationship
 */
function islandora_workflow_set_object_relationship($object_id, $relationship_in, $target) {
  //init
  $islandora_workflow_namespace='info:islandora/islandora-system:def/islandora_workflow#';
  module_load_include('raw.inc', 'islandora_fedora_api'); //for getting an object
  $apim_object= new FedoraAPIM();
  
  //get existing relationshp
  $relationships=$apim_object->getRelationships($object_id, $islandora_workflow_namespace . $relationship_in);
  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);
  $relationship_elements=$relationships_parser->getElementsByTagNameNS($islandora_workflow_namespace, $relationship_in);
  $current_relationship=NULL;
  $relationship=$relationship_elements->item(0);
  if (!empty($relationship)) {
    foreach ($relationship->childNodes as $relationship_text_node) {
      $current_relationship=$relationship_text_node->nodeValue;
      //clear current relationship
      $apim_object->purgeRelationship($object_id, $islandora_workflow_namespace . $relationship_in, $current_relationship, array('isLiteral' => 'true'));
    }
  }
  //set new relationship
  $apim_object->addRelationship($object_id, $islandora_workflow_namespace . $relationship_in, $target, array('isLiteral' => 'true'));
}

/**
 * This function will get the collection that the indicated object is a member of
 * @param string $object_id
 *   The id of the object to get the parent of
 * @return mixed $parent
 *   The id of the collection object that contains the $object_id object or FALSE if no parent found
 */
function islandora_workflow_get_object_parent($object_id) {
  //init
  $parent_relationship='isMemberOf';
  $parent_relationship_namespace='info:fedora/fedora-system:def/relations-external#';
  module_load_include('raw.inc', 'islandora_fedora_api'); //for getting an object
  $apim_object= new FedoraAPIM();

  $relationships=$apim_object->getRelationships($object_id, $parent_relationship_namespace . $parent_relationship);
  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);
  $relationship_elements=$relationships_parser->getElementsByTagNameNS($parent_relationship_namespace, $parent_relationship);

  $parent=FALSE;
  $relationship=$relationship_elements->item(0);
  
  if (!empty($relationship)) {
    $parent=$relationship->getAttributeNS('http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'resource');
    //cut out 'info:fedora/'
    if (substr($parent, 0, 12)=='info:fedora/') {
      $parent=substr($parent, 12, strlen($parent));
    }
  }
  return $parent;
}

/**
 * This function is for general workflow permission retrieval on individual 
 * non-collection objects.
 * Init is broken up for performance.
 * @param string $object_id
 *   The object that we are looking for permissions on
 * @param object $user_in
 *   The Drupal user whose permissions we are checking or null for the current user
 * @return mixed
 *   Iether the string of the highest available permission to the user on the object or 
 *   false if there are no permissions
 */
function islandora_workflow_user_object_permission_check($object_id, $user_in=null) {
  //init
  $manager_permissions=islandora_workflow_get_current_users_collections('islandora_workflow_Manager');
  $parent=islandora_workflow_get_object_parent($object_id); 
  
  //if the user has manager level access return true
  if (isset($manager_permissions[$parent])) {
    return 'islandora_workflow_Manager';
  }
  
  //more init
  $editor_permissions=islandora_workflow_get_current_users_collections('islandora_workflow_Editor');
  $workflow_state=islandora_workflow_get_object_workflow_state($object_id);
  
  //editor level
  if (isset($editor_permissions[$parent]) && ($workflow_state=='submitted' || $workflow_state=='created')) {
    return 'islandora_workflow_Editor';
  }
  
  //even more init
  $submitter_permissions=islandora_workflow_get_current_users_collections('islandora_workflow_Submitter');
  $creator=islandora_workflow_get_object_creator($object_id);
  $user_name=NULL;
  //get user name
  if ($user_in==NULL) {
    global $user;
    $user_name=$user->name;
  }
  else {
    $user_name=$user_in->name;
  }
  
  //submitter level
  if (isset($submitter_permissions[$parent]) && ($workflow_state=='created' || $workflow_state=='rejected' ) && $user_name==$creator) {
    return 'islandora_workflow_Submitter';
  }
  
  //permission denied
  return FALSE;
}