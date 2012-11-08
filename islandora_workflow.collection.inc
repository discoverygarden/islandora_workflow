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
function islandora_workflow_get_collections() {
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
    $collections[$collection_pid] = array(
      'label' => $result['label'],
      'workflow_tracking' => 'TRUE' == $result['workflow_tracking'],
    );
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
  if (!empty($workflow_relationships)) {
    return (in_array('TRUE', $workflow_relationships['is_tracked_by_workflow']));
  }
  return FALSE;

}
