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
 * @return $collection_list: an associated array of collection pids and names
 */
 function get_all_current_users_collections() {
  $collection_list=array();
  
  
   if (user_access('islandora_workflow_Administrator'))//admin show all
   	$collection_list=get_all_collections();
   elseif(user_access('islandora_workflow_Manager'))//manager show permitted
   	$collection_list=get_all_collections();
   	$permitted_collections='';//do a querry on table
   	foreach ($collection_list as $collection_pid=> $collection_name)
   		if (!in_array($collection_name,$permitted_collections))
   			unset($collection_list[$collection_pid]);
   
  
  return $collection_list; 
 }

 /**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @return $collection_list: an associated array of collection pids and names
 */
function get_all_collections() {
  
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
 *
 * @param string $itql_query
 * @return <type>
 */
function islandora_workflow_get_related_objects($itql_query) {
  module_load_include('inc', 'fedora_repository', 'CollectionClass');
  $collectionClass = new CollectionClass();
  $query_results = $collectionClass->getRelatedItems(NULL, $itql_query);
  $list_of_objects = islandora_workflow_itql_to_array($query_results);
  return $list_of_objects;
}

/**
 *
 * @param <array> $query_results
 * @return string
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