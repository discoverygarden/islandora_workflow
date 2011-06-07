<?php
/**
 * @file
 * @author: William Panting
 * @date: 02/06/2011
 * This is a collection of helper functions and such for the islandora_workflow module.
 */


/**
 * This function uses the get_all_collections() function and in the case of a manager
 * the drupal database table 'islandora_workflow_permissions' to compile a list of 
 * collections that the user has manager level and above permissions to
 * TODO: build the drupal db query and test on manager level user
 * @return $collection_list: an associated array of collection pids and names
 */
function islandora_workflow_get_current_users_collections() {
  $collection_list=array();
  
  if (user_access('islandora_workflow_Administrator')) {//admin show all
    $collection_list=islandora_workflow_get_all_collections();
  }
  elseif (user_access('islandora_workflow_Manager')) {//manager show permitted
    $collection_list=islandora_workflow_get_all_collections();
    $permitted_collections='';//do a querry on table
    foreach ($collection_list as $collection_pid => $collection_name) {
      if (!in_array($collection_name, $permitted_collections)) {
        unset($collection_list[$collection_pid]);
      }
    }
  }
  return $collection_list; 
}

/**
 * This function returns the islandora_workflow permissions that the indicated role/user 
 * has with the indicated collection
 * @param string $collection_id:
 * @return array $perm_list: list of permissions the current user has with the indicated  
 */
function islandora_workflow_get_collection_permissions_by_role() {
  
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