<?php
/**
 * @file
 * @author: William Panting
 * @date: 02/06/2011
 * This is a collection of helper functions and such for the islandora_workflow module.
 */


/**
 * This function uses the get_all_collections() function in the case of an admin user
 * but otherwise will use the supplied permission string to search
 * the drupal database table 'islandora_workflow_permissions' to compile a list of 
 * collections that the user has the supplied permission to 
 * This is also handy to determin if the user has any permissions at that lvl:
 * if (islandora_workflow_get_current_users_collections($perm)!=FALSE) {}
 * 
 * @param string $permission_level: the permission level we are looking for
 * @return $collection_list: an associated array of collection pids and names, 
 * @return boolean: a false boolean if they don't have the indicated perms
 */
function islandora_workflow_get_current_users_collections($permission_level='islandora_workflow_Manager') {
  //init
  $permitted_collections=array();
  $collection_list=array();
  if (user_access('islandora_workflow_Administrator') || user_access('administer')) {//admin show all
    $collection_list=islandora_workflow_get_all_collections();
  }
  elseif (user_access($permission_level)) {//manager show permitted
    $collection_list=islandora_workflow_get_all_collections();
    //figure out owned collections
    //role perms
    $users_role_collection_permissions=islandora_workflow_get_current_users_role_permissions();
    foreach ($users_role_collection_permissions as $role_id => $collections) {
      foreach ($collections as $collection_id => $permission) {
        if ($permission==$permission_level) {
          $permitted_collections[]=$collection_id;
        }
      }
    }
    //user perms
    $users_user_collection_permissions=islandora_workflow_get_current_users_user_permissions();
    foreach ($users_user_collection_permissions as $role_id => $collections) {
      foreach ($collections as $collection_id => $permission) {
        if ($permission==$permission_level) {
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
 * @return array $perm_list: list of permissions that roles have with the form $perm_list[$role_id][$collection_id][1...4]=perms
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
 * @return array $perm_list: list of all permisions that users have  with the form  $perm_list[$user_id][$collection_id][1...4]=perms
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
 * @param string $perm the permission to search for
 * @return array $users the list of users with the permission in the form users[user_id]=user_name
 */
function user_access_list($perm = NULL) {
  $uids = FALSE;
    if (isset($perm)) {
      $query = "SELECT u.uid, u.name FROM {users} u, {users_roles} ur, {permission} p
      WHERE        u.uid = ur.uid
       AND          ur.rid = p.rid
      AND          p.perm LIKE '%%%s%'
      ";
      $result = db_query($query, $perm);
      $users = array();
      while ($u = db_fetch_object($result)) {
        $users[$u->uid]=$u->name;
    }
  }
  return $users;
}

/**
 * This function query's drupal's DB for all the collection permission entries related to the current user
 * Most usefull for building the work portal, and also used for getting user's owned collections
 * @return array $perm_list: list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id][1...4]=perms
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
 * @return array $perm_list: list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_current_users_user_permissions() {
  //do islandora_workflow_get_all_role_permissions with restrictions on the roles to the list of roles the current user has
  $perm_list=array();
  global $user;//use $user->roles to get an array $roles[$role_id][$role_name]  $role_id is int
  
  //build query
  $query='SELECT user, collection, permission FROM {islandora_workflow_user_permissions}';
  $user_id=$user->uid;
  $query=$query . ' WHERE user="' . $user_id . '"';
  
  //build return array
  while ($row=db_fetch_array($results)) {
    $perm_list[$row['user']][$row['collection']] = $row['permission'];
  }
  return $perm_list;  
}

 /**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @return $collection_list: an associated array of collection pids and names
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
 * @param string $itql_query:
 * @return array $list_of_objects:
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
 * @param string $query_results: 
 * @return array $list_of_objects: 
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
 * @return array $privledged_roles: The list of roles
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
 * @return array $privledged_users: The list of users
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
 * @param string $object_pid: the fedora pid of the object to modify
 * @return string $result: the return mesage from fedora
 */
function islandora_workflow_init_object_workflow_datastream($object_pid) {
  //init
  module_load_include('inc', 'fedora_repository', 'api/fedora_item'); //for getting an object
  $object= new Fedora_Item($object_pid);
  $default_xml_file_name=drupal_get_path('module', 'islandora_workflow') . '/start_workflow.xml';
  //$default_xml_file_handle=fopen($default_xml_file, "r");
  //$default_workflow_datastream=fread($default_xml_file_handle, filesize($default_xml_file_name));
  //fclose($default_xml_file_handle);
  $workflow_parser = new DOMDocument();
  $workflow_parser->load($default_xml_file_name);
  global $user;
  //get elements to populate dynamicaly
  $assignee_array=$workflow_parser->getElementsByTagName('');
  $last_modified_by_array=$workflow_parser->getElementsByTagName('');
  $when_last_modified_array=$workflow_parser->getElementsByTagName('');
  
  //populate the elements
  foreach ($assignee_array as $assignee_element) {
    //create text node
    $assignee=$workflow_parser->createTextNode($user->uid);
    //insert text node
    $assignee_element->appendChild($assignee);
  }
  foreach ($last_modified_by_array as $last_modified_by_element) {
    //create text node
    $last_modified_by=$workflow_parser->createTextNode($user->uid);
    //insert text node
    $last_modified_by_element->appendChild($last_modified_by);
  }
  foreach ($when_last_modified_array as $when_last_modified_element) {
    //create text node
    $when_last_modified=$workflow_parser->createTextNode(date('c'));//date('c') is supposed to give back an xml friendly string
    //insert text node
    $when_last_modified_element->appendChild($when_last_modified);
  }
  
  $result=$object->add_datastream_from_string($workflow_parser->saveXML(), 'islandora_workflow', 'workflow', 'text/xml','X','Adding islandora_workflow datastream(workflow tracking).');
  return $result;
}

/**
 * This function will move a fedora object to the rejected state
 * @param string $object_pid: the fedora pid of the object to modify
 * @return string $result: the return mesage from fedora
 */
function islandora_workflow_reject_object($object_pid) {
  
}

/**
 * This function will move a fedora object to the approved state
 * @param string $object_pid: the fedora pid of the object to modify
 * @return string $result: the return mesage from fedora
 */
function islandora_workflow_approve_object($object_pid) {
  
} 

/**
 * This function will move a fedora object to the published state
 * @param string $object_pid: the fedora pid of the object to modify
 * @return string $result: the return mesage from fedora
 */
function islandora_workflow_publish_object($object_pid) {
  
}

/**
 * This function will move a fedora object to the submitted state
 * @param string $object_pid: the fedora pid of the object to modify
 * @return string $result: the return mesage from fedora
 */
function islandora_workflow_submit_object($object_pid) {
  
}

/**
 * This function will assign the object to the current user state
 * @param string $object_pid: the fedora pid of the object to modify
 * @return string $result: the return mesage from fedora
 */
function islandora_workflow_claim_object($object_pid) {
  global $user;
  islandora_workflow_assign_object($object_pid, $user->uid);
}

/**
 * This function will assign the fedora object to the indicated user
 * @param string $object_pid: the fedora pid of the object to modify
 * @return string $result: the return mesage from fedora
 */
function islandora_workflow_assign_object($object_pid, $user_id) {
  
}

/**
 * This function will get the current state of workflow for an object.
 * @param string $object_id: the fedora pid of the object to check the status of
 * @return string $state: The string reperesenting the state of the object in the workflow
 */
 function islandora_workflow_get_object_state($object_id) {
   $state='';
   
   return $state;
 }