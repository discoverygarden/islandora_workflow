<?php
/**
 * @file
 *   This include file is meant to hold all helper functions regarding collections
 */

/**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @TODO:  use this function to replace islandora_workflow_get_all_collections()
 * @author
 *   William Panting
 * @return $collection_list
 *   an associated array of collection pids and names
 */
function islandora_workflow_get_collections() {
  module_load_include('inc', 'islandora_fedora_api', 'islandora_fedora_api.risearch');
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow');

  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/collection_workflow_states_query.txt';
  $query_file_handle = fopen($query_file_name, "r");
  $query_string = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);

  $query_return_string = query_resource_index($query_string);

  if ($query_return_string) {
    $query_return_dom = DOMDocument::loadXML($query_return_string);
  }

  //format query results for use (xml->array)
  $collection_data = array();
  $result_nodes = $query_return_dom->getElementsByTagName('result');
  $result_nodes_length = $result_nodes->length;
  if ($result_nodes_length > 0) {
    for ($i=0; $i < $result_nodes_length; $i++) {
      $result_node = $result_nodes->item($i);

      $label_element = $result_node->getElementsByTagName('label')->item(0);
      $collection_pid_element = $result_node->getElementsByTagName('collection_object')->item(0);
      $workflow_tracking_element = $result_node->getElementsByTagName('workflow_tracking')->item(0);

      $collection_object_uri = $collection_pid_element->getAttribute('uri');
      //info:fedora/ is 12 chars
      $collection_object_pid = substr($collection_object_uri, 12, strlen($collection_object_uri));//need to extract the pid

      $collection_data[$collection_object_pid] = array();

      if ($label_element) {
        $label = $label_element->nodeValue;
        if ($label) {
          //the label may not be pupulated
          $collection_data[$collection_object_pid]['label'] = $label;
        }
      }
      if ($workflow_tracking_element) {
        $workflow_tracking = $workflow_tracking_element->nodeValue;
        if ($workflow_tracking && $workflow_tracking == 'TRUE') {
          $collection_data[$collection_object_pid]['workflow_tracking'] = TRUE;
        }
        else {
          $collection_data[$collection_object_pid]['workflow_tracking'] = FALSE;
        }
      }
    }
  }
  //strip out non-applicable collections via namespace
  $collection_data = islandora_workflow_limit_collections_by_namespace($collection_data);
  return $collection_data;
}

/**
 * Set the RELS-EXT relationship on a collection for
 * workflow tracking to TRUE
 * @param $object_pid
 *   The object to set the relationship on
 * @return
 *   fedora http response
 * @todo test
 * df
 */
function islandora_workflow_track_collection_in_workflow($object_pid) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow');
  return (islandora_workflow_set_object_relationship($object_pid, 'is_tracked_by_workflow', 'TRUE'));
}

/**
 * Set the RELS-EXT relationship on a collection for
 * workflow tracking to FALSE
 *   The object to set the relationship on
 * @return
 *   fedora http response
 *   * @todo test
 */
function islandora_workflow_stop_tracking_collection_in_workflow($object_pid) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow');
  return (islandora_workflow_set_object_relationship($object_pid, 'is_tracked_by_workflow', 'FALSE'));
}

/**
 *This function checks if the collection is tracked in workflow.
 *The function checks for a RELS-EXT that may not exist.
 *@param $object_pid
 *  The pid of the object to check on
 *@return boolean
 *  TRUE if the collection is tracked in workflow
 *  FALSE if the collection is not tracked in workflow
 *@todo: look at creating api func for checking if rel exists
 */
function islandora_workflow_is_collection_workflow_tracked($object_pid) {

  module_load_include('inc', 'islandora_workflow', 'islandora_workflow');
  $relationship_in = 'is_tracked_by_workflow';
  $islandora_workflow_namespace='info:islandora/islandora-system:def/islandora_workflow#';
  module_load_include('raw.inc', 'islandora_fedora_api'); //for getting an object
  $apim_object= new FedoraAPIM();

  //get existing relationshp
  $relationships = $apim_object->getRelationships($object_pid, $islandora_workflow_namespace . $relationship_in, $subject);


  $relationships_parser = new DOMDocument();
  $relationships_parser->loadXML($relationships->data);
  $relationship_elements = $relationships_parser->getElementsByTagNameNS($islandora_workflow_namespace, $relationship_in);
  $current_relationship = NULL;
  $relationship = $relationship_elements->item(0);
  if (!empty($relationship)) {
    return TRUE;
  }
  return FALSE;
}