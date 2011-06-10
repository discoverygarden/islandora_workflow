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
 * @param string $permission_level: the permission level we are looking for
 * @return $collection_list: an associated array of collection pids and names, 
 * @return boolean: a false boolean if they don't have manger or higher perms
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
    $users_collection_permissions=islandora_workflow_get_current_users_permissions();
    foreach ($users_collection_permissions as $role_id => $collections) {
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
 * This function query's drupal's DB for all the collection permission entries
 * Used when displaying the current state for the permission page
 * @return array $perm_list: list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_all_permissions() {
  $perm_list=array();
  
  //build query
  $query='SELECT role, collection, permission FROM {islandora_workflow_permissions}';
  $results=db_query($query);
  
  //build return array
  while ($row=db_fetch_array($results)) {
    $perm_list[$row['role']][$row['collection']]=$row['permission'];
  }

  return $perm_list;
}

/**
 * This function query's drupal's DB for all the collection permission entries related to the current user
 * Most usefull for building the work portal, and also used for getting user's owned collections
 * @return array $perm_list: list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_current_users_permissions() {
  //do islandora_workflow_get_all_permissions with restrictions on the roles to the list of roles the current user has
  $perm_list=array();
  global $user;//use $user->roles to get an array $roles[$role_id][$role_name]  $role_id is int
  
  //build query
  $query='SELECT role,collection,permission FROM {islandora_workflow_permissions}';
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
    $perm_list[$row['role']]=array($row['collection'] => $row['permission']
    );
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
  $collectionClass = new CollectionClass();
  $query_results = $collectionClass->getRelatedItems(NULL, $itql_query);
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
 *This function is here for testing perposes
 *TODO: remove this function when it is no longer of immediate use 
 * */
function islandora_workflow_insert_test_data() {
  $obj=array(
  'role' => '1',
  'collection' => 'Awill:257',
  'permission' => 'islandora_workflow_Administrator'
  );
  drupal_write_record('islandora_workflow_permissions', $obj);
  
  $obj=array(
  'role' => '2',
  'collection' => 'Awill:257',
  'permission' => 'islandora_workflow_Submitter'
  );
  drupal_write_record('islandora_workflow_permissions', $obj);
  
  $obj=array(
  'role' => '3',
  'collection' => 'Awill:257',
  'permission' => 'islandora_workflow_Manager'
  );
  drupal_write_record('islandora_workflow_permissions', $obj);
}