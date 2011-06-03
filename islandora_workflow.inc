<?php
// $Id: islandora_workflow.inc, v 1 $
/**
 * @file
 * @author: William Panting
 * @date: 02/06/2011
 * This is a collection of helper functions and such for the islandora_workflow module.
 */

/**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @return an associated array of collection pids and names
 */
function get_all_collections() {
  $collection_list=array();
  
  //read in the itql query for getting collections
  $query_file_name='collection_query.txt';
  $query_file_handle=fopen($query_file_name, 'r');
  $query_string=fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  
  return $collection_list;  
}

